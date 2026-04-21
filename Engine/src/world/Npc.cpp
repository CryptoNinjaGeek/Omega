#include <world/Npc.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <geometry/Object.h>
#include <render/Shader.h>
#include <system/Log.h>
#include <world/Heightmap.h>
#include <world/PropColliders.h>

namespace omega {
namespace world {

namespace {

// Normalize an angle to (-pi, pi]. Used to take the shortest yaw delta when
// steering.
float wrapAngle(float a) {
  constexpr float twoPi = glm::two_pi<float>();
  while (a >  glm::pi<float>()) a -= twoPi;
  while (a <= -glm::pi<float>()) a += twoPi;
  return a;
}

// Clone an individual mesh the way the Basic / Outdoor demos do: share the
// VAO/VBO handles of the prototype, copy materials and bounding sphere so
// frustum culling still works on each instance.
std::shared_ptr<geometry::Object> cloneMeshShared(
    const std::shared_ptr<geometry::Object>& proto) {
  auto copy = std::make_shared<geometry::Object>(
      proto->getVAO(), proto->getVBO(), proto->getCount(), proto->getType());
  copy->setName(proto->name());
  copy->setTextures(proto->getTextures());
  if (auto mat = proto->getMaterial()) copy->setMaterial(*mat);
  if (const auto& sphere = proto->boundingSphere()) {
    copy->setBoundingSphere(*sphere);
  }
  return copy;
}

// Walk the prototype tree recursively, flattening each mesh into the output
// list along with the composed parent-to-local matrix. Also clones the tree
// so the NPC owns an independent ObjectNode structure.
//
// `parentMat` is the composed matrix from root down to the current node. We
// multiply it by the node's local `mat` before descending — this is exactly
// what Scene::render would do if it honored ObjectNode::mat (it currently
// doesn't, which is why we capture the composed matrix per-mesh here).
geometry::ObjectNodePtr cloneAndFlatten(
    const geometry::ObjectNodePtr& node, const glm::mat4& parentMat,
    std::vector<NpcMeshEntry>& out) {
  if (!node) return nullptr;

  auto copy = std::make_shared<geometry::ObjectNode>();
  const glm::mat4 localToModel = parentMat * node->mat;
  copy->mat = node->mat;

  copy->meshes.reserve(node->meshes.size());
  for (const auto& m : node->meshes) {
    auto cloned = cloneMeshShared(m);
    copy->meshes.push_back(cloned);
    out.push_back({cloned, localToModel});
  }
  copy->children.reserve(node->children.size());
  for (const auto& c : node->children) {
    copy->children.push_back(cloneAndFlatten(c, localToModel, out));
  }
  return copy;
}

// Approximate the whole-animal bounding sphere in model space by taking the
// union of each mesh's local sphere, pushed through its `localMatrix`. Used
// only to auto-derive the horizontal collision radius.
void computeModelSphere(const std::vector<NpcMeshEntry>& entries,
                        glm::vec3& centerOut, float& radiusOut) {
  bool haveAny = false;
  glm::vec3 minP(0.0f), maxP(0.0f);
  for (const auto& e : entries) {
    const auto& bs = e.mesh->boundingSphere();
    if (!bs) continue;
    const glm::vec3 c = glm::vec3(e.localMatrix * glm::vec4(bs->center, 1.0f));
    const float r = bs->radius;
    const glm::vec3 lo = c - glm::vec3(r);
    const glm::vec3 hi = c + glm::vec3(r);
    if (!haveAny) {
      minP = lo;
      maxP = hi;
      haveAny = true;
    } else {
      minP = glm::min(minP, lo);
      maxP = glm::max(maxP, hi);
    }
  }
  if (!haveAny) {
    centerOut = glm::vec3(0.0f);
    radiusOut = 0.5f;
    return;
  }
  centerOut = 0.5f * (minP + maxP);
  radiusOut = 0.5f * glm::length(maxP - minP);
  if (radiusOut <= 0.0f) radiusOut = 0.5f;
}

}  // namespace

std::shared_ptr<Npc> Npc::spawn(
    const geometry::ObjectNodePtr& proto,
    std::shared_ptr<render::Shader> shader,
    std::shared_ptr<const Heightmap> heightmap,
    const glm::vec2& spawnXZ,
    const NpcParams& params,
    std::string name) {
  if (!proto) {
    OMEGA_LOG_WARN("npc", "Npc::spawn called with null prototype");
    return nullptr;
  }

  auto npc = std::make_shared<Npc>();
  npc->name_ = std::move(name);
  npc->params_ = params;
  npc->heightmap_ = std::move(heightmap);
  npc->root_ = cloneAndFlatten(proto, glm::mat4(1.0f), npc->meshEntries_);

  if (!npc->root_ || npc->meshEntries_.empty()) {
    OMEGA_LOG_WARN("npc",
                   "Npc::spawn produced no meshes (prototype tree empty?)");
    return nullptr;
  }

  // Bake the shader into every cloned mesh — same pattern as the demos.
  if (shader) {
    for (auto& e : npc->meshEntries_) {
      e.mesh->setShader(shader);
    }
  }

  // Approximate model-space extents → default collision radius if caller
  // didn't supply one. Scale into world units via params_.scale. We keep
  // only the horizontal component (footprint) rather than the full 3D
  // radius so tall animals (giraffe) don't carry a huge collision sphere.
  computeModelSphere(npc->meshEntries_, npc->modelSphereCenter_,
                     npc->modelSphereRadius_);
  if (npc->params_.collisionRadius <= 0.0f) {
    npc->params_.collisionRadius =
        0.6f * npc->modelSphereRadius_ * npc->params_.scale;
  }

  // Spawn pose: XZ from caller, Y pulled from the terrain. yaw is randomized
  // later; picking 0 for the first frame is fine — steering will rotate us.
  npc->position_ = glm::vec3(spawnXZ.x, 0.0f, spawnXZ.y);
  npc->targetXZ_ = spawnXZ;
  npc->snapToGround();
  npc->yaw_ = 0.0f;
  npc->desiredYaw_ = 0.0f;
  npc->pauseTimer_ = 0.0f;
  npc->animTime_ = 0.0f;
  npc->currentBobY_ = 0.0f;

  // Commit the starting pose to the meshes so the first frame is not drawn at
  // the origin while update() is still waiting for its first `dt > 0`.
  npc->applyTransformsToMeshes();
  return npc;
}

void Npc::pickNewTarget(std::mt19937& rng) {
  std::uniform_real_distribution<float> angle(0.0f, glm::two_pi<float>());
  std::uniform_real_distribution<float> radius(
      0.3f * params_.wanderRadius, params_.wanderRadius);
  std::uniform_real_distribution<float> pause(
      params_.minPauseTime, params_.maxPauseTime);

  // Try a few candidates; accept the first one that lands on walkable terrain.
  // If none qualify, fall back to the last candidate (prevents deadlock on a
  // tight ridge and moves the NPC roughly in the right direction anyway).
  glm::vec2 pick = targetXZ_;
  for (int attempt = 0; attempt < 8; ++attempt) {
    const float a = angle(rng);
    const float r = radius(rng);
    glm::vec2 p(position_.x + std::cos(a) * r,
                position_.z + std::sin(a) * r);

    // Fence clamp — prevents the pack drifting off the island edge.
    if (params_.halfExtentXZ.x > 0.0f && params_.halfExtentXZ.y > 0.0f) {
      p.x = std::clamp(p.x,
                       params_.fenceCenter.x - params_.halfExtentXZ.x,
                       params_.fenceCenter.x + params_.halfExtentXZ.x);
      p.y = std::clamp(p.y,
                       params_.fenceCenter.z - params_.halfExtentXZ.y,
                       params_.fenceCenter.z + params_.halfExtentXZ.y);
    }

    pick = p;
    if (!heightmap_ || params_.maxWalkableSlope <= 0.0f) break;
    const glm::vec3 n = heightmap_->normalAtWorld(p.x, p.y);
    if ((1.0f - n.y) <= params_.maxWalkableSlope) break;
  }
  targetXZ_ = pick;
  pauseTimer_ = 0.0f;
  // Rebuild desiredYaw toward the new target so the NPC pivots in place at
  // the start of the leg instead of skating sideways.
  const glm::vec2 toTarget = targetXZ_ - glm::vec2(position_.x, position_.z);
  if (glm::length(toTarget) > 1e-4f) {
    // +Z is the model's forward axis in our rotation convention (see
    // composeWorldMatrix), so atan2(x, z) maps a world direction to the
    // yaw angle that points the model that way.
    desiredYaw_ = std::atan2(toTarget.x, toTarget.y);
  }
  // Don't overwrite yaw_ — we interpolate toward desiredYaw_ in stepSteering.
  // `pause(rng)` here only runs to stay in sync with the RNG sequence; the
  // actual pause is (re)scheduled when we arrive at the target.
  (void)pause(rng);
}

void Npc::stepSteering(float dt) {
  // Rotate toward desiredYaw at turnSpeed (rad/s). Clamp the delta to avoid
  // overshoot at large dt.
  const float yawErr = wrapAngle(desiredYaw_ - yaw_);
  const float maxStep = params_.turnSpeed * dt;
  const float appliedStep = std::clamp(yawErr, -maxStep, maxStep);
  yaw_ = wrapAngle(yaw_ + appliedStep);

  // Advance along the current heading only if we're roughly pointed at the
  // target (dot > 0.2). This gives a tiny "pivot in place first" feel and
  // avoids the animal skating sideways while it turns.
  const glm::vec2 here(position_.x, position_.z);
  const glm::vec2 toTarget = targetXZ_ - here;
  const float dist = glm::length(toTarget);
  if (dist < 1e-4f) return;
  const glm::vec2 dir = toTarget / dist;

  const glm::vec2 forward(std::sin(yaw_), std::cos(yaw_));
  const float alignment = glm::dot(forward, dir);
  if (alignment < 0.2f) return;

  const float step = std::min(params_.moveSpeed * dt, dist);
  glm::vec2 next = here + forward * step;

  // Slope gate: if the step lands on terrain that's steeper than our walkable
  // threshold, stop moving this frame. The next `pickNewTarget` call will
  // re-roll — we don't try to slide.
  if (heightmap_ && params_.maxWalkableSlope > 0.0f) {
    const glm::vec3 n = heightmap_->normalAtWorld(next.x, next.y);
    if ((1.0f - n.y) > params_.maxWalkableSlope) {
      // Force a re-plan — nudge the accept distance down so the next
      // update() tick treats us as "arrived" and picks a new target.
      pauseTimer_ = 0.0f;
      targetXZ_ = here;
      return;
    }
  }
  position_.x = next.x;
  position_.z = next.y;
}

void Npc::snapToGround() {
  if (!heightmap_ || !heightmap_->valid()) return;
  const float y = heightmap_->heightAtWorld(position_.x, position_.z);
  position_.y = y + params_.groundOffset;
}

void Npc::update(float dt, std::mt19937& rng) {
  animTime_ += dt;

  // `resolveCollisions` sets this flag when a static-obstacle push actually
  // moved us this frame. Consume it here so the NPC immediately picks a new
  // wander target instead of steering back toward `targetXZ_` (which still
  // points past the tree/rock it just bounced off). We also clear any pending
  // pause so the retarget isn't deferred by an "I just arrived" timer.
  //
  // `pickNewTarget` draws a uniform random angle, so occasionally the new
  // target will still be on the obstacle side — that's fine: the next frame's
  // resolve will flag us again and we'll re-roll. Over one or two retargets
  // the pack drifts clear.
  if (pendingRetarget_) {
    pendingRetarget_ = false;
    pauseTimer_ = 0.0f;
    pickNewTarget(rng);
  }

  // If we're paused, tick the timer and use the idle bob.
  if (pauseTimer_ > 0.0f) {
    pauseTimer_ -= dt;
    if (pauseTimer_ <= 0.0f) {
      pickNewTarget(rng);
    }
  } else {
    stepSteering(dt);
    // Arrived? Schedule a pause then a new target.
    const glm::vec2 here(position_.x, position_.z);
    const float dist = glm::length(targetXZ_ - here);
    if (dist < params_.waypointAcceptDistance) {
      std::uniform_real_distribution<float> pause(
          params_.minPauseTime, params_.maxPauseTime);
      pauseTimer_ = pause(rng);
      if (pauseTimer_ <= 0.0f) {
        pickNewTarget(rng);
      }
    }
  }

  // Ground-follow every frame. Snapping is analytical and cheap; there's no
  // physics body underneath the NPC so we never need velocity integration in
  // Y. If we ever want NPCs jumping, mirror the TerrainCameraController jump
  // code here.
  snapToGround();

  // Procedural "walk bob": up/down sinusoid when moving, tiny breathing when
  // idle. Keeps the scene from feeling static while we wait for real node
  // animation playback. The same signal is fed to leg-phase visuals in the
  // future — for now it lives entirely in the Y offset written by
  // applyTransformsToMeshes().
  const float ampScale = (pauseTimer_ > 0.0f) ? params_.idleBobScale : 1.0f;
  currentBobY_ = params_.bobAmplitude * ampScale *
                 std::sin(animTime_ * params_.bobFrequency);
}

void Npc::resolveCollisions(
    const std::vector<std::shared_ptr<Npc>>& npcs,
    const PropColliderSet* obstacles) {
  const int n = static_cast<int>(npcs.size());
  for (int i = 0; i < n; ++i) {
    auto& a = npcs[i];
    if (!a) continue;
    for (int j = i + 1; j < n; ++j) {
      auto& b = npcs[j];
      if (!b) continue;

      const glm::vec2 pa(a->position_.x, a->position_.z);
      const glm::vec2 pb(b->position_.x, b->position_.z);
      const glm::vec2 d = pb - pa;
      const float distSq = glm::dot(d, d);
      const float minDist = a->params_.collisionRadius +
                            b->params_.collisionRadius;
      if (distSq >= minDist * minDist || distSq <= 1e-8f) continue;

      const float dist = std::sqrt(distSq);
      const float penetration = (minDist - dist);
      const glm::vec2 n = d / dist;

      // Split the correction evenly. More sophisticated rules (mass ratios,
      // immovable animals) can weight this differently later.
      const glm::vec2 push = n * (penetration * 0.5f);
      a->position_.x -= push.x;
      a->position_.z -= push.y;
      b->position_.x += push.x;
      b->position_.z += push.y;
    }
  }

  // Static-obstacle push. Runs AFTER the pairwise pass so an NPC that was
  // just shoved into a tree by a neighbour is still popped back out before
  // the frame is committed. `resolveXZ` is a no-op when the set is empty so
  // passing a freshly-constructed PropColliderSet is harmless.
  //
  // When a push actually moved the NPC we also rotate its desiredYaw toward
  // the push direction and flag `pendingRetarget_`. Without that, the NPC's
  // existing `targetXZ_` still points past the obstacle; stepSteering would
  // walk it straight back into the same tree on the next tick, resolve would
  // push it out again, and we'd loop forever. The retarget flag makes the
  // next `update()` pick a fresh destination; the yaw bias nudges the new
  // pick away from the obstacle even before `pickNewTarget` runs.
  if (obstacles) {
    for (auto& npc : npcs) {
      if (!npc) continue;
      const glm::vec2 xz(npc->position_.x, npc->position_.z);
      const glm::vec2 adj =
          obstacles->resolveXZ(xz, npc->params_.collisionRadius);
      const glm::vec2 delta = adj - xz;
      if (glm::dot(delta, delta) > 1e-6f) {
        // yaw convention: forward is +Z, atan2(x, z) maps a direction to the
        // yaw angle that points the model that way. Matches stepSteering and
        // pickNewTarget.
        const glm::vec2 escape = glm::normalize(delta);
        npc->desiredYaw_ = std::atan2(escape.x, escape.y);
        npc->pendingRetarget_ = true;
      }
      npc->position_.x = adj.x;
      npc->position_.z = adj.y;
    }
  }

  // Re-snap to ground for everyone we nudged horizontally. Cheap — this is a
  // single bilinear sample per NPC — and covers both the pairwise push above
  // and the static-obstacle push just performed.
  for (auto& npc : npcs) {
    if (npc) npc->snapToGround();
  }
}

glm::mat4 Npc::composeWorldMatrix() const {
  // Model forward is +Z; rotate by yaw around +Y so atan2(x, z) aligns.
  glm::mat4 m(1.0f);
  m = glm::translate(m, glm::vec3(position_.x,
                                  position_.y + currentBobY_,
                                  position_.z));
  m = glm::rotate(m, yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
  m = glm::scale(m, glm::vec3(params_.scale));
  return m;
}

void Npc::applyTransformsToMeshes() {
  const glm::mat4 world = composeWorldMatrix();
  for (auto& e : meshEntries_) {
    e.mesh->setModel(world * e.localMatrix);
  }
}

}  // namespace world
}  // namespace omega
