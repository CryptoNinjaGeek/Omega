#include <geometry/Portal.h>
#include <render/PortalFramebuffer.h>
#include <cmath>

using namespace omega::geometry;

Portal::Portal() {
  updateVectors();
}

Portal::Portal(const glm::vec3& position, const glm::vec3& normal, float width,
               float height)
    : position_(position), normal_(normal), width_(width), height_(height) {
  updateVectors();
}

void Portal::setNormal(const glm::vec3& normal) {
  normal_ = glm::normalize(normal);
  updateVectors();
}

void Portal::setSize(float width, float height) {
  width_ = width;
  height_ = height;
}

void Portal::updateVectors() {
  // Normalize normal vector
  normal_ = glm::normalize(normal_);

  // Calculate up vector (default to world up, then adjust)
  glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

  // If normal is parallel to world up, use forward as reference
  if (std::abs(glm::dot(normal_, worldUp)) > 0.99f) {
    worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
  }

  // Calculate right vector (normal × up)
  right_ = glm::normalize(glm::cross(normal_, worldUp));

  // Recalculate up vector (right × normal) to ensure orthogonality
  up_ = glm::normalize(glm::cross(right_, normal_));
}

glm::mat4 Portal::getTransform() const {
  // Build a local-to-world (L2W) matrix for this portal.
  //
  // Local frame convention:
  //   +X_local → right_     (across the portal face)
  //   +Y_local → up_        (vertical on the portal face)
  //   +Z_local → -normal_   ("through" the portal — the direction you'd walk
  //                          to exit out the far side, i.e. opposite of the
  //                          face that the normal points out of)
  //   origin_local → position_
  //
  // NOTE on GLM indexing: glm::mat4[col][row], so m[0] is the first *column*.
  // A proper L2W stores basis vectors as COLUMNS (not rows). The previous
  // implementation stored them as rows, which produced the transpose of the
  // intended rotation and broke downstream portal math — see PortalCamera.
  glm::mat4 transform(1.0f);
  transform[0] = glm::vec4(right_,    0.0f);    // column 0: world-space right
  transform[1] = glm::vec4(up_,       0.0f);    // column 1: world-space up
  transform[2] = glm::vec4(-normal_,  0.0f);    // column 2: "into" the portal
  transform[3] = glm::vec4(position_, 1.0f);    // column 3: world position
  return transform;
}

void Portal::getCorners(glm::vec3 corners[4]) const {
  // Calculate half dimensions
  float halfWidth = width_ * 0.5f;
  float halfHeight = height_ * 0.5f;

  // Calculate corner positions relative to portal center
  // Top-left, top-right, bottom-right, bottom-left
  corners[0] = position_ + (-right_ * halfWidth) + (up_ * halfHeight);
  corners[1] = position_ + (right_ * halfWidth) + (up_ * halfHeight);
  corners[2] = position_ + (right_ * halfWidth) + (-up_ * halfHeight);
  corners[3] = position_ + (-right_ * halfWidth) + (-up_ * halfHeight);
}

bool Portal::isPointInFront(const glm::vec3& point) const {
  // Check if point is in front of portal plane
  glm::vec3 toPoint = point - position_;
  float dot = glm::dot(toPoint, normal_);
  return dot > 0.0f;
}

float Portal::distanceToPlane(const glm::vec3& point) const {
  // Calculate signed distance from point to portal plane
  glm::vec3 toPoint = point - position_;
  return glm::dot(toPoint, normal_);
}

bool Portal::isVisibleFrom(const glm::vec3& position) const {
  // Check if portal is facing the given position
  glm::vec3 toPortal = position_ - position;
  float dot = glm::dot(glm::normalize(toPortal), normal_);
  
  // Portal is visible if the position is in front of it (dot < 0 means position is behind portal normal)
  // Actually, we want to check if portal is facing the position, so dot should be negative
  // (normal points away from portal, so if position is in front, dot is negative)
  return dot < 0.0f;
}

glm::vec4 Portal::getClippingPlane() const {
  // Clipping plane: normal points away from portal (opposite of portal normal)
  // This prevents seeing through the back of the portal
  glm::vec3 planeNormal = -normal_;  // Negative normal (points away from portal)
  float distance = glm::dot(planeNormal, position_);
  
  return glm::vec4(planeNormal, distance);
}

