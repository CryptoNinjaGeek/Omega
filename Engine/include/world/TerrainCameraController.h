#pragma once

// TerrainCameraController — analytical ground-follow for a camera walking on
// a Heightmap. Wraps a shared Heightmap and, each frame, adjusts the camera's
// world Y so it sits at `eyeHeight` above the sampled terrain. Horizontal
// motion is filtered against `maxWalkableSlope` so the player can't climb
// cliffs.
//
// Everything here is pure math + a weak `Camera` dependency (for the
// convenience `updateCamera` method). The core helpers `resolvePosition` and
// `filterHorizontal` take plain vectors so they can be unit-tested without
// building an OpenGL context or a real CameraFPS.
//
// Collision is analytical, not physics-backed, for the reasons in
// `docs/outdoor_scene_plan.md §3.3`: terrain height is one bilinear sample, a
// few ALU ops, and the FPS camera doesn't need contact forces. If a future
// project wants dynamic bodies on the terrain, ReactPhysics3D's
// HeightFieldShape can consume the same Heightmap without disturbing this
// controller.

#include <memory>

#include <system/Global.h>

#include <glm/glm.hpp>

namespace omega {
namespace render {
class Camera;
}
namespace world {

class Heightmap;
class PropColliderSet;

struct TerrainCameraParams {
  // Offset from the terrain surface up to the camera's eye. 1.7 m is roughly
  // human eye height at 1 world unit = 1 m; tune to taste.
  float eyeHeight{1.7f};

  // Maximum vertical delta the camera is allowed to apply in a single frame.
  // Prevents the camera snapping through walls of height when walking onto a
  // cliff face. Too small feels like the camera is "stuck"; 0.5 m per frame
  // at 60 fps = 30 m/s max vertical speed, which is plenty for normal play.
  float stepHeight{0.5f};

  // Exponential smoothing rate (1/sec) applied to the Y coordinate. Higher
  // values are snappier; 10.0 means the error halves in roughly 70 ms.
  // 0 disables smoothing — the camera locks exactly to the surface each
  // frame (useful for tests).
  float groundSmoothing{10.0f};

  // Slope threshold above which the controller refuses to move the camera
  // horizontally. Slope is `1 - normal.y`, so 0 is flat, 1 is a vertical
  // wall. 0.7 corresponds to a ~45° incline.
  float maxWalkableSlope{0.7f};

  // Initial upward velocity (world units / second) applied by `requestJump`.
  // With the default gravity below, 6.0 reaches ~1.8 m — comfortable "hop over
  // a log" height, not "leap a wall". Set to 0 to disable jumping without
  // touching the caller.
  float jumpSpeed{6.0f};

  // Downward acceleration (world units / second²) integrated while airborne.
  // Deliberately higher than real Earth gravity (9.81) so the jump arc feels
  // responsive rather than floaty — standard platformer tuning. Drop to
  // 9.81 for a "moon gravity" feel.
  float gravity{20.0f};

  // Horizontal actor radius used when pushing the camera out of obstacle
  // spheres stored in the attached PropColliderSet. A person is roughly
  // 0.3–0.4 m wide at the shoulders; we go a touch larger so the camera stops
  // before clipping the outer silhouette of a trunk. Set to 0 (together with
  // leaving obstacles unset) to disable prop collision entirely.
  float actorRadius{0.4f};
};

class OMEGA_EXPORT TerrainCameraController {
 public:
  TerrainCameraController() = default;

  // Build with a shared heightmap. The controller holds a shared_ptr so the
  // caller doesn't have to keep the heightmap alive separately.
  explicit TerrainCameraController(std::shared_ptr<const Heightmap> heightmap,
                                   TerrainCameraParams params = {});

  void setHeightmap(std::shared_ptr<const Heightmap> heightmap) {
    heightmap_ = std::move(heightmap);
  }
  const std::shared_ptr<const Heightmap>& heightmap() const { return heightmap_; }

  void setParams(TerrainCameraParams params) { params_ = params; }
  const TerrainCameraParams& params() const { return params_; }

  // Sample the terrain height at (x, z) in world space. Returns
  // `verticalOffset` (i.e. sea level) when no heightmap is bound so callers
  // never see a bogus world Y.
  float groundHeight(const glm::vec2& xz) const;

  // Is the terrain at (x, z) walkable given current params?
  bool isWalkable(const glm::vec2& xz) const;

  // Filter a horizontal move: if the destination is too steep, return the
  // origin (i.e. block motion). If moving between two reachable cells crosses
  // an unwalkable one, we still accept — the MVP doesn't do sub-step ray
  // marches; step-height + smoothing handle the feel.
  glm::vec2 filterHorizontal(const glm::vec2& previousXZ,
                             const glm::vec2& desiredXZ) const;

  // Compute a new world-space position for the camera given the previous
  // position and the desired XZ after input. Applies ground-follow with
  // smoothing + step-height clamp; ignores the caller's incoming Y so the
  // FPS camera never "floats" or falls through. Pure function — no Camera
  // required, no state mutated.
  glm::vec3 resolvePosition(const glm::vec3& previous,
                            const glm::vec2& desiredXZ,
                            float dt) const;

  // Convenience: read the camera's current position, apply filterHorizontal
  // + resolvePosition (or ballistic integration if airborne), write it back.
  // Call once per frame after the FPS movement handler has already applied
  // keyboard input. Not const because it mutates the jump state.
  void updateCamera(render::Camera& camera, float dt);

  // Request a jump. Ignored if the camera is already airborne, so holding
  // the key doesn't produce double-jumps. Edge-triggered by the caller
  // (e.g. from a key-down event, not a poll).
  void requestJump();

  // True while the camera is in the middle of a ballistic arc — i.e. a jump
  // has been requested and it hasn't landed yet. Exposed mostly so tests can
  // observe the state machine; gameplay code shouldn't need to consult it.
  bool isAirborne() const { return airborne_; }

  // Attach (or detach with nullptr) the set of static sphere obstacles to
  // push the camera out of each frame. `updateCamera` queries the set after
  // applying the terrain slope filter but before integrating ground-follow
  // Y, so an obstacle that overlaps the camera never produces a single-frame
  // "walked through it" visible position. The controller keeps a shared_ptr
  // so the caller does not have to guarantee lifetime separately.
  void setObstacles(std::shared_ptr<const PropColliderSet> obstacles) {
    obstacles_ = std::move(obstacles);
  }
  const std::shared_ptr<const PropColliderSet>& obstacles() const {
    return obstacles_;
  }

 private:
  // Ground-follow Y from a (possibly already-filtered / already-pushed) XZ.
  // Pulled out of `resolvePosition` so `updateCamera` can call it after the
  // prop-collider pass without the inner slope filter silently reverting the
  // push. Pure: no state mutation.
  float groundFollowY(float previousY, const glm::vec2& xz, float dt) const;

  std::shared_ptr<const Heightmap> heightmap_;
  std::shared_ptr<const PropColliderSet> obstacles_;
  TerrainCameraParams params_;

  // Ballistic jump state. `verticalVelocity_` is integrated under
  // `params_.gravity` every frame while `airborne_` is true; the camera lands
  // (airborne cleared, velocity zeroed) as soon as its Y re-crosses the
  // ground-plus-eye-height line with non-positive velocity. Kept on the
  // controller rather than the Camera so the Camera stays dumb and so plain
  // `Camera` works in demos without a physics body.
  float verticalVelocity_{0.0f};
  bool airborne_{false};
};

}  // namespace world
}  // namespace omega
