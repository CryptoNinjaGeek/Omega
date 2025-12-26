#pragma once

#include <system/Global.h>
#include <render/Camera.h>
#include <glm/glm.hpp>
#include <memory>

namespace omega {
namespace render {

/**
 * PortalViewCamera - Temporary camera wrapper for portal rendering
 * Allows rendering scene from portal perspective using custom view matrix
 */
class OMEGA_EXPORT PortalViewCamera : public Camera {
public:
  PortalViewCamera(std::shared_ptr<Camera> baseCamera, const glm::mat4& customView);
  
  // Override view matrix to use custom portal view
  glm::mat4 viewMatrix() const;
  glm::mat4x4 projectionMatrix();

private:
  std::shared_ptr<Camera> baseCamera_;
  glm::mat4 customView_;
};

}  // namespace render
}  // namespace omega

