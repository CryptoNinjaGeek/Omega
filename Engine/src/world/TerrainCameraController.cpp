#include <world/TerrainCameraController.h>

#include <algorithm>
#include <cmath>

#include <render/Camera.h>
#include <world/Heightmap.h>
#include <world/PropColliders.h>

namespace omega {
namespace world {

namespace {

// Exponential smoothing factor for a given rate over dt. Returns the fraction
// of the error to close *this* frame. rate=0 disables smoothing (snap to
// target). The framework is standard "lerp by 1-exp(-rate*dt)" which is
// frame-rate independent.
float smoothFactor(float rate, float dt) {
  if (rate <= 0.0f) return 1.0f;
  if (dt <= 0.0f) return 0.0f;
  return 1.0f - std::exp(-rate * dt);
}

}  // namespace

TerrainCameraController::TerrainCameraController(
    std::shared_ptr<const Heightmap> heightmap, TerrainCameraParams params)
    : heightmap_(std::move(heightmap)), params_(params) {}

float TerrainCameraController::groundHeight(const glm::vec2& xz) const {
  if (!heightmap_ || !heightmap_->valid()) {
    // Callers expect a finite number even without a heightmap — otherwise a
    // stray NaN would propagate into the camera and Scene would happily draw
    // nothing. Fall back to sea level.
    return heightmap_ ? heightmap_->transform().verticalOffset : 0.0f;
  }
  return heightmap_->heightAtWorld(xz.x, xz.y);
}

bool TerrainCameraController::isWalkable(const glm::vec2& xz) const {
  if (!heightmap_ || !heightmap_->valid()) return true;
  // Normalized UV space corresponds to the heightmap's own surface; that's
  // what the slope is defined against. Transform scaling is already folded
  // into sampleNormal (which central-differences gradientWorld).
  const glm::vec2 uv = glm::vec2(
      (xz.x - heightmap_->transform().origin.x) /
          std::max(heightmap_->transform().horizontalScale, 1e-6f),
      (xz.y - heightmap_->transform().origin.y) /
          std::max(heightmap_->transform().horizontalScale, 1e-6f));
  const float slope = heightmap_->sampleSlope(uv.x, uv.y);
  return slope <= params_.maxWalkableSlope;
}

glm::vec2 TerrainCameraController::filterHorizontal(
    const glm::vec2& previousXZ, const glm::vec2& desiredXZ) const {
  if (!heightmap_ || !heightmap_->valid()) return desiredXZ;
  return isWalkable(desiredXZ) ? desiredXZ : previousXZ;
}

float TerrainCameraController::groundFollowY(
    float previousY, const glm::vec2& xz, float dt) const {
  const float groundY = groundHeight(xz);
  const float targetY = groundY + params_.eyeHeight;

  // Smooth toward target, then clamp the absolute per-frame delta so a sharp
  // terrain feature doesn't teleport the camera.
  const float alpha = smoothFactor(params_.groundSmoothing, dt);
  float nextY = previousY + (targetY - previousY) * alpha;

  const float maxStep = std::max(params_.stepHeight, 0.0f);
  const float delta = nextY - previousY;
  if (delta >  maxStep) nextY = previousY + maxStep;
  if (delta < -maxStep) nextY = previousY - maxStep;
  return nextY;
}

glm::vec3 TerrainCameraController::resolvePosition(
    const glm::vec3& previous, const glm::vec2& desiredXZ, float dt) const {
  const glm::vec2 filteredXZ = filterHorizontal(
      glm::vec2(previous.x, previous.z), desiredXZ);
  const float nextY = groundFollowY(previous.y, filteredXZ, dt);
  return glm::vec3(filteredXZ.x, nextY, filteredXZ.y);
}

void TerrainCameraController::updateCamera(render::Camera& camera, float dt) {
  const glm::vec3 prev = camera.position();
  const glm::vec2 desiredXZ(prev.x, prev.z);

  // Horizontal filter runs unconditionally: you can't walk through a cliff
  // mid-jump any more than you can while grounded. Y is handled differently
  // depending on whether we're airborne.
  glm::vec2 filteredXZ = filterHorizontal(desiredXZ, desiredXZ);

  // Prop-collider push-out. Has to happen AFTER the slope filter (so a sphere
  // near a cliff doesn't hand the camera back to an unwalkable tile) and
  // BEFORE ground-follow Y (so the Y we integrate matches the corrected XZ).
  // `resolveXZ` is a no-op when obstacles_ is empty, so this costs almost
  // nothing when no props have been wired up.
  if (obstacles_ && params_.actorRadius > 0.0f) {
    filteredXZ = obstacles_->resolveXZ(filteredXZ, params_.actorRadius);
  }

  const float groundY = groundHeight(filteredXZ);
  const float floorY = groundY + params_.eyeHeight;

  if (airborne_) {
    // Symplectic-ish Euler: velocity first, then position. At typical 60+ FPS
    // and our gravity magnitude the error is visually invisible — this is a
    // cosmetic jump, not a physics sim.
    verticalVelocity_ -= params_.gravity * dt;
    float nextY = prev.y + verticalVelocity_ * dt;

    // Landing test: once we've descended to or through the eye-height line
    // and velocity is no longer upward, snap to the floor and clear the
    // airborne flag so the normal ground-follow path takes over next frame.
    if (nextY <= floorY && verticalVelocity_ <= 0.0f) {
      nextY = floorY;
      verticalVelocity_ = 0.0f;
      airborne_ = false;
    }
    camera.setPositon(glm::vec3(filteredXZ.x, nextY, filteredXZ.y));
    return;
  }

  // Grounded path: smoothed ground-follow against the already-pushed XZ.
  // We deliberately avoid calling `resolvePosition` here because that helper
  // re-applies `filterHorizontal(previous, desired)` internally, which would
  // compare prev-vs-post-obstacle-push and potentially revert the push if
  // the push crosses an unwalkable tile. `groundFollowY` takes an XZ as-is.
  const float nextY = groundFollowY(prev.y, filteredXZ, dt);
  camera.setPositon(glm::vec3(filteredXZ.x, nextY, filteredXZ.y));
}

void TerrainCameraController::requestJump() {
  // Ignore if already airborne so holding SPACE doesn't chain-jump. `jumpSpeed
  // <= 0` lets callers disable jumping by setting it to zero without a
  // branch at the call site.
  if (airborne_ || params_.jumpSpeed <= 0.0f) return;
  verticalVelocity_ = params_.jumpSpeed;
  airborne_ = true;
}

}  // namespace world
}  // namespace omega
