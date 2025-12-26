#include <render/PortalViewCamera.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace omega::render;

PortalViewCamera::PortalViewCamera(std::shared_ptr<Camera> baseCamera, 
                                   const glm::mat4& customView)
    : Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
      baseCamera_(baseCamera),
      customView_(customView) {
  // Extract camera position from view matrix
  // View matrix = inverse(camera transform), so position = -inverse(view)[3].xyz
  glm::mat4 invView = glm::inverse(customView);
  position_ = glm::vec3(invView[3]);
  
  // Also extract front direction for lighting
  glm::vec3 forward = -glm::vec3(customView[0][2], customView[1][2], customView[2][2]);
  front_ = glm::normalize(forward);
  
  // Copy projection matrix from base camera (will be overridden if framebuffer aspect differs)
  if (baseCamera_) {
    projection_matrix_ = baseCamera_->projectionMatrix();
  }
}

glm::mat4 PortalViewCamera::viewMatrix() const {
  return customView_;
}

glm::mat4x4 PortalViewCamera::projectionMatrix() {
  // Always return the stored projection matrix
  // This allows PortalRenderer to override it with framebuffer-specific aspect ratio
  return projection_matrix_;
}

