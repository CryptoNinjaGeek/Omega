#pragma once

// PropColliderSet — an analytical world-space sphere collection used to stop
// the player camera and wandering NPCs from walking through placed static
// props (trees, rocks, etc.).
//
// This is not a physics system. The Outdoor demo intentionally keeps the
// camera and the NPCs off of ReactPhysics3D (see the long comment in
// Demo/Outdoor/main.cpp about the base `Camera` + TerrainCameraController
// pairing), so prop collision stays analytical to match. Each collider is a
// sphere in world coordinates with a precomputed radius; `resolveXZ` pushes a
// proposed XZ position out of every overlapping sphere in a handful of
// iterations.
//
// How this is normally populated
//   • The demo places a tree or rock via `placeModel` — that calls
//     `mesh->setModel(world)` on each mesh in the cloned ObjectNode tree.
//   • Immediately after placement the demo calls
//     `obstacles_->addFromPlacedNode(instance, radiusScale)`, which reads
//     each mesh's world-space bounding sphere via `Object::worldBoundingSphere`
//     and scales it down toward the visible trunk / core.
//
// Why XZ-only (ignore Y)?
//   Actors on the terrain are modelled as vertical cylinders walking on a
//   heightmap — if an XZ-coincident sphere exists at all, the actor is either
//   about to enter the trunk (tree) or the rock. Skipping the vertical overlap
//   check costs nothing in correctness for this scene and avoids a per-sphere
//   Y comparison. Cliff-overhang edge cases (an actor directly under a rock
//   resting on a ledge above) are acknowledged but not in scope for Phase 1.
//
// See docs/outdoor_scene_plan.md §3.3 for the full rationale behind analytical
// rather than physics-backed collision for static vegetation.

#include <cstddef>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <geometry/ObjectTree.h>
#include <system/Global.h>

namespace omega {
namespace world {

// A single static world-space sphere collider. Populated once at scene build
// time and read every frame.
struct PropCollider {
  glm::vec3 center{0.0f};
  float     radius{0.0f};
};

class OMEGA_EXPORT PropColliderSet {
 public:
  PropColliderSet() = default;

  void clear() { colliders_.clear(); }

  // Append a single sphere. `radius <= 0` is silently dropped so callers can
  // pass the product of a bounding sphere and a shrink factor without having
  // to guard the degenerate cases (a flat plane mesh, a missing sphere).
  void add(const glm::vec3& center, float radius);

  // Walk an ObjectNode tree (one whose meshes have already had their world
  // model matrix set — see `placeModel` in the Outdoor demo) and append one
  // sphere per mesh. Each mesh's local bounding sphere is transformed into
  // world space via `Object::worldBoundingSphere()` and scaled by
  // `radiusScale`.
  //
  // `radiusScale < 1` is the common case: the visible bounds of a tree model
  // usually include the canopy, and the trunk/core is smaller. ~0.35 works
  // well for conifer-shaped trees and ~0.55 for rocks, but this is a demo-
  // side tuning concern and not enforced here.
  void addFromPlacedNode(const geometry::ObjectNodePtr& node,
                         float radiusScale);

  // Push a proposed XZ position out of every overlapping stored sphere.
  // The actor is modelled as a vertical cylinder of horizontal radius
  // `actorRadius`; each sphere imposes a keep-out radius of
  // (sphere.radius + actorRadius) in XZ.
  //
  // `iterations` runs the pairwise push multiple times so corrections against
  // one sphere that leave the actor overlapping a neighbour still resolve.
  // Two passes covers typical two-tree clusters cheaply; bump if dense
  // foliage pushes the actor into a third tree.
  //
  // Returns the corrected XZ. If no spheres overlap (the common case every
  // frame for most actors), the input is returned unchanged.
  glm::vec2 resolveXZ(const glm::vec2& proposedXZ,
                      float actorRadius,
                      int iterations = 2) const;

  std::size_t size() const { return colliders_.size(); }
  const std::vector<PropCollider>& colliders() const { return colliders_; }

 private:
  std::vector<PropCollider> colliders_;
};

}  // namespace world
}  // namespace omega
