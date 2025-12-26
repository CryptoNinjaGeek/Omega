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
  // Create transform matrix from portal orientation
  glm::mat4 transform = glm::mat4(1.0f);

  // Set rotation part (right, up, -normal form the basis)
  transform[0][0] = right_.x;
  transform[1][0] = right_.y;
  transform[2][0] = right_.z;

  transform[0][1] = up_.x;
  transform[1][1] = up_.y;
  transform[2][1] = up_.z;

  transform[0][2] = -normal_.x;  // Negative normal (forward direction)
  transform[1][2] = -normal_.y;
  transform[2][2] = -normal_.z;

  // Set translation
  transform[3][0] = position_.x;
  transform[3][1] = position_.y;
  transform[3][2] = position_.z;

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

