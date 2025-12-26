#pragma once

#include <system/Global.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace omega {
namespace render {
class PortalFramebuffer;
class Shader;
class Camera;
}  // namespace render
namespace geometry {

/**
 * Portal - Represents a portal surface in the world
 * Portals are paired together to create connections between locations
 */
class OMEGA_EXPORT Portal {
public:
  Portal();
  Portal(const glm::vec3& position, const glm::vec3& normal, float width = 2.0f,
         float height = 2.0f);

  // Position and orientation
  void setPosition(const glm::vec3& position) { position_ = position; }
  glm::vec3 getPosition() const { return position_; }

  void setNormal(const glm::vec3& normal);
  glm::vec3 getNormal() const { return normal_; }

  void setSize(float width, float height);
  float getWidth() const { return width_; }
  float getHeight() const { return height_; }

  // Get portal transform matrix
  glm::mat4 getTransform() const;

  // Get portal corners (for rendering portal surface)
  void getCorners(glm::vec3 corners[4]) const;

  // Get up and right vectors
  glm::vec3 getUp() const { return up_; }
  glm::vec3 getRight() const { return right_; }

  // Linked portal (destination)
  void setLinkedPortal(std::shared_ptr<Portal> portal) { linkedPortal_ = portal; }
  std::shared_ptr<Portal> getLinkedPortal() const { return linkedPortal_; }

  // Framebuffer for rendering portal view
  void setFramebuffer(std::shared_ptr<render::PortalFramebuffer> fb) {
    framebuffer_ = fb;
  }
  std::shared_ptr<render::PortalFramebuffer> getFramebuffer() const {
    return framebuffer_;
  }

  // Portal visibility
  void setVisible(bool visible) { visible_ = visible; }
  bool isVisible() const { return visible_; }

  // Portal enabled state
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

  // Check if point is in front of portal
  bool isPointInFront(const glm::vec3& point) const;

  // Get distance from point to portal plane
  float distanceToPlane(const glm::vec3& point) const;

private:
  void updateVectors();

  glm::vec3 position_{0.0f, 0.0f, 0.0f};
  glm::vec3 normal_{0.0f, 0.0f, -1.0f};  // Default: facing forward (-Z)
  glm::vec3 up_{0.0f, 1.0f, 0.0f};
  glm::vec3 right_{1.0f, 0.0f, 0.0f};

  float width_{2.0f};
  float height_{2.0f};

  std::shared_ptr<Portal> linkedPortal_{nullptr};
  std::shared_ptr<render::PortalFramebuffer> framebuffer_{nullptr};

  bool visible_{true};
  bool enabled_{true};
};

}  // namespace geometry
}  // namespace omega

