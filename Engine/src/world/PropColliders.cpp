#include <world/PropColliders.h>

#include <algorithm>
#include <cmath>

#include <geometry/Object.h>

namespace omega {
namespace world {

void PropColliderSet::add(const glm::vec3& center, float radius) {
  // A zero-or-negative radius produces no meaningful keep-out volume and
  // would be returned straight back to the caller during resolveXZ (distSq <
  // (0+actorR)² is still possible for distSq exactly 0). Drop it here so the
  // hot loop doesn't have to special-case it.
  if (radius <= 0.0f) return;
  colliders_.push_back({center, radius});
}

void PropColliderSet::addFromPlacedNode(
    const geometry::ObjectNodePtr& node, float radiusScale) {
  if (!node || radiusScale <= 0.0f) return;
  // Each mesh already has its world model matrix applied by the demo's
  // `placeModel` before this is called, so `worldBoundingSphere` returns the
  // placed-in-world sphere. A mesh without bounds (optional is empty) is
  // silently skipped — the `project_object_cloner_bounding_sphere` memory
  // lists this as a foot-gun for cloning, so we favour skipping over an
  // incorrect zero-radius collider.
  for (const auto& mesh : node->meshes) {
    if (!mesh) continue;
    if (auto worldSphere = mesh->worldBoundingSphere()) {
      add(worldSphere->center, worldSphere->radius * radiusScale);
    }
  }
  for (const auto& child : node->children) {
    addFromPlacedNode(child, radiusScale);
  }
}

glm::vec2 PropColliderSet::resolveXZ(
    const glm::vec2& proposedXZ, float actorRadius, int iterations) const {
  if (colliders_.empty() || actorRadius < 0.0f) return proposedXZ;

  glm::vec2 p = proposedXZ;
  const int nIter = std::max(iterations, 1);

  for (int iter = 0; iter < nIter; ++iter) {
    bool moved = false;
    for (const auto& c : colliders_) {
      const glm::vec2 cxz(c.center.x, c.center.z);
      const glm::vec2 d = p - cxz;
      const float distSq = glm::dot(d, d);
      const float minDist = c.radius + actorRadius;
      if (distSq >= minDist * minDist) continue;

      if (distSq <= 1e-8f) {
        // Actor is exactly on the sphere centre — dividing by the distance
        // would NaN. Push along +X deterministically; the next iteration
        // against this or another sphere will separate further if needed.
        p.x += minDist;
        moved = true;
        continue;
      }

      const float dist = std::sqrt(distSq);
      const float penetration = minDist - dist;
      const glm::vec2 dir = d / dist;
      p += dir * penetration;
      moved = true;
    }
    // Early-out: a full pass that changed nothing means we're clear of every
    // sphere. Saves an iteration in the common "one sphere only" case.
    if (!moved) break;
  }
  return p;
}

}  // namespace world
}  // namespace omega
