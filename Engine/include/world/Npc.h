#pragma once

// Npc — an autonomous "walking animal" living on a Heightmap-backed terrain.
//
// An Npc owns a cloned `ObjectNode` tree loaded from a GLB/FBX model (typically
// one of Kenney's animal packs under `:/models/animals/`). It keeps its own
// world position / yaw / velocity and each frame:
//
//   1. Steers toward a randomly-picked wander target,
//   2. Snaps to the terrain surface via the shared `Heightmap`,
//   3. Applies a small procedural walk-bob so the critter "animates" even
//      though the engine does not yet play back skeletal/node keyframes.
//
// Model loading uses `Loader::loadModelPreTransformed` so that GLBs with
// multi-part hierarchies (body / legs / tail / head) render as a single
// coherent animal. Without pre-transformed vertices the engine's renderer
// would stack every body part at the model origin because Scene::render does
// not walk the ObjectNode hierarchy transforms. The animal meshes themselves
// still carry keyframed node animations inside the .glb files (walk / idle /
// run / eat / dance), and hooking those up to real playback is listed as
// future work — the procedural bob is the MVP stand-in.
//
// Collision is sphere-sphere and resolved by a static helper over a flat NPC
// list (typical pack size in the Outdoor demo is ~20, so an O(N²) pass is
// fine). Each NPC also keeps a horizontal "collision radius" that is
// automatically derived from the prototype's local bounding sphere so a
// giraffe doesn't share a cat's footprint.

#include <memory>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <geometry/ObjectTree.h>
#include <system/Global.h>

namespace omega {
namespace render {
class Shader;
}  // namespace render
namespace world {

class Heightmap;
class PropColliderSet;

struct NpcParams {
  // Peak forward speed in world units/sec. Tuned small — cats/dogs/foxes feel
  // right at ~1.5–2 units/s when 1 unit ≈ 1 m. Bigger animals (elephant) can
  // set this higher at spawn time.
  float moveSpeed{1.8f};

  // Turn rate in rad/s toward the desired heading. ~3 rad/s is roughly
  // 170°/s, which looks snappy without being instantaneous.
  float turnSpeed{3.0f};

  // When picking a new wander target, sample a random XZ offset in this
  // radius from the current position. Larger → longer legs of the path,
  // smaller → hops around a small area.
  float wanderRadius{18.0f};

  // "Close enough to count as arrived" radius. Once we're inside it we pick
  // a new target (or pause first).
  float waypointAcceptDistance{1.0f};

  // After arriving, stand still for a uniform-random duration in this
  // inclusive range before picking the next target. Makes the pack feel less
  // like a rigid conga line.
  float minPauseTime{0.5f};
  float maxPauseTime{3.0f};

  // Horizontal collision radius. Populated automatically from the model's
  // bounding sphere if left <= 0 at spawn time. Callers can override at
  // spawn for "please don't let this elephant sink into the fox".
  float collisionRadius{0.0f};

  // Uniform model scale. Large for elephants/giraffes, small for caterpillars.
  float scale{1.0f};

  // Extra world-Y offset from the sampled terrain height. 0 means the model
  // origin sits exactly on the ground. Useful if a model has its feet below
  // its pivot and you need to raise it a few cm.
  float groundOffset{0.0f};

  // Vertical amplitude and angular frequency of the procedural "walk bob".
  // Only applied while the NPC is moving; idle NPCs use a much smaller
  // breathing amplitude (see idleBobScale).
  float bobAmplitude{0.06f};
  float bobFrequency{10.0f};

  // Fraction of `bobAmplitude` used when idle. Gives a subtle "still alive"
  // effect during the between-target pauses.
  float idleBobScale{0.15f};

  // When > 0, the NPC rejects a wander target whose terrain slope exceeds
  // this value (1 - normal.y). Keeps the pack off cliffs. 0 disables the
  // check, which is what small / agile animals (cats, birds) want.
  float maxWalkableSlope{0.55f};

  // World-space AABB the wander targets are clamped inside — a soft fence so
  // the pack doesn't drift off the island edge. Set both to the same XZ +
  // something like half the terrain extent. If `halfExtentXZ.x` is <= 0 the
  // clamp is skipped.
  glm::vec3 fenceCenter{0.0f};
  glm::vec2 halfExtentXZ{0.0f, 0.0f};
};

// Per-mesh bookkeeping captured once at spawn: the cloned mesh and the
// local-space matrix it should sit at inside the NPC's own frame. Each frame
// we write `worldTransform * localMatrix` into the mesh so Scene::render
// picks up the updated pose.
struct NpcMeshEntry {
  std::shared_ptr<geometry::Object> mesh;
  glm::mat4 localMatrix{1.0f};
};

class OMEGA_EXPORT Npc {
 public:
  Npc() = default;

  // Build a new NPC from an already-parsed prototype tree (typically loaded
  // once per species via `Loader::loadModelPreTransformed` and cached in the
  // caller). The proto is deep-cloned — the prototype is left untouched so a
  // single GLB can back many NPCs that share VAO/VBO handles but carry
  // independent transforms.
  //
  // `spawnXZ` is an XZ position in world space; the NPC snaps to the terrain
  // surface at that point on the first update. `heightmap` is kept by
  // shared_ptr.
  //
  // Returns nullptr if proto is empty / invalid.
  static std::shared_ptr<Npc> spawn(
      const geometry::ObjectNodePtr& proto,
      std::shared_ptr<render::Shader> shader,
      std::shared_ptr<const Heightmap> heightmap,
      const glm::vec2& spawnXZ,
      const NpcParams& params = {},
      std::string name = {});

  // Cloned, NPC-owned tree. The Outdoor demo passes this into Scene::add()
  // exactly once after spawn — Scene then traverses it every frame.
  geometry::ObjectNodePtr root() const { return root_; }

  const std::string& name() const { return name_; }
  const glm::vec3&  position() const { return position_; }
  float collisionRadius() const { return params_.collisionRadius; }
  float yaw() const { return yaw_; }
  bool  isIdle() const { return pauseTimer_ > 0.0f; }

  // Advance steering + ground-follow + procedural animation. Does NOT push
  // transforms into the meshes — call `applyTransformsToMeshes` afterwards
  // (usually after `resolveCollisions` has nudged positions around).
  void update(float dt, std::mt19937& rng);

  // Pairwise sphere-sphere collision pass. For every pair whose horizontal
  // distance drops below (r_a + r_b) we push both NPCs outward by half the
  // penetration along the XZ separation direction. Y is left alone — each
  // NPC snaps back to the ground on its next update.
  //
  // When `obstacles` is non-null, each NPC is also pushed out of every
  // overlapping static world-space sphere stored in the set (trees, rocks,
  // etc.). Static push happens AFTER the pairwise NPC-NPC pass so a cluster
  // of critters can't squeeze into the same tree by each splitting the
  // penetration with its neighbour. The final ground-snap covers both types
  // of correction in a single re-sample.
  //
  // Expected pack size is a handful to a few dozen, so the O(N²) inner loop
  // is cheaper than a spatial index. If NPC counts ever reach the hundreds,
  // swap this for a uniform-grid broadphase.
  static void resolveCollisions(
      const std::vector<std::shared_ptr<Npc>>& npcs,
      const PropColliderSet* obstacles = nullptr);

  // Bake the NPC's current (position, yaw, bob) pose into every mesh's model
  // matrix. Cheap — one mat4 multiply per mesh.
  void applyTransformsToMeshes();

 private:
  // Helpers —
  void pickNewTarget(std::mt19937& rng);
  void stepSteering(float dt);
  void snapToGround();
  glm::mat4 composeWorldMatrix() const;

  std::string name_;
  NpcParams params_;

  geometry::ObjectNodePtr root_;
  std::vector<NpcMeshEntry> meshEntries_;
  std::shared_ptr<const Heightmap> heightmap_;

  // Approximate model-space bounding sphere over the whole animal (the union
  // of each mesh's local sphere transformed into model space). Used to
  // compute a default collisionRadius when the caller didn't set one.
  glm::vec3 modelSphereCenter_{0.0f};
  float     modelSphereRadius_{1.0f};

  glm::vec3 position_{0.0f};
  glm::vec2 targetXZ_{0.0f};
  float     yaw_{0.0f};         // around +Y
  float     desiredYaw_{0.0f};
  float     pauseTimer_{0.0f};  // > 0 → standing still
  float     animTime_{0.0f};
  float     currentBobY_{0.0f};
  // Set by `resolveCollisions` when the NPC was pushed out of a static
  // obstacle (tree, rock, etc.) this frame. The next `update()` consumes it
  // to force an immediate `pickNewTarget` — otherwise the NPC would keep
  // walking straight back into the same obstacle it just bounced off of.
  bool      pendingRetarget_{false};
};

}  // namespace world
}  // namespace omega
