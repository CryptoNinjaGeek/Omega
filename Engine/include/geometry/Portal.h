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

  // Linked portal (destination) - kept for backward compatibility
  void setLinkedPortal(std::shared_ptr<Portal> portal) { linkedPortal_ = portal; }
  std::shared_ptr<Portal> getLinkedPortal() const { return linkedPortal_; }

  // Destination portal (new doorway-based system)
  void setDestination(std::shared_ptr<Portal> dest) { destination_ = dest; }
  std::shared_ptr<Portal> getDestination() const { return destination_; }

  // Check if this portal is a mirror (destination == this)
  bool isMirror() const { return destination_ && destination_.get() == this; }

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

  // Portal open/closed state (for doors)
  void setOpen(bool open) { isOpen_ = open; }
  bool isOpen() const { return isOpen_; }

  // Portal passability (can entities walk through?)
  void setPassable(bool passable) { isPassable_ = passable; }
  bool isPassable() const { return isPassable_; }

  // Mirror overlay settings
  void setMirrorOverlay(bool enabled, float intensity = 0.5f) {
    hasMirrorOverlay_ = enabled;
    mirrorIntensity_ = intensity;
  }
  bool hasMirrorOverlay() const { return hasMirrorOverlay_; }
  float getMirrorIntensity() const { return mirrorIntensity_; }

  // Visibility and culling helpers
  bool isVisibleFrom(const glm::vec3& position) const;
  glm::vec4 getClippingPlane() const;  // For portal culling

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

  std::shared_ptr<Portal> linkedPortal_{nullptr};  // Legacy: kept for backward compatibility
  std::shared_ptr<Portal> destination_{nullptr};   // New: doorway-based destination
  std::shared_ptr<render::PortalFramebuffer> framebuffer_{nullptr};

  bool visible_{true};
  bool enabled_{true};
  bool isOpen_{true};           // Is the doorway open? (for doors)
  bool isPassable_{true};       // Can entities walk through this portal?
  bool hasMirrorOverlay_{false}; // Optional mirror effect overlay
  float mirrorIntensity_{0.5f};  // Mirror overlay intensity (0.0 = transparent, 1.0 = full mirror)
};

}  // namespace geometry
}  // namespace omega

