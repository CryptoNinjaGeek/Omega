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

  // Destination portal. A portal self-linked to itself (destination == this)
  // is a mirror — see isMirror(). Portals loaded via PortalSceneLoader get
  // this set from the scene JSON's `destination` field (or left null if the
  // portal is never rendered/traversed).
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

  // Mirror overlay settings (Phase 1.5).
  //
  // When enabled, `PortalRenderer::renderPortalSurface` uploads these values
  // to `portal.fs` which tints the destination view by `intensity` toward
  // `tint`. Default tint = (1, 1, 1) so that enabling the overlay without
  // a bespoke tint behaves as a no-op — demos that want a classic greyscale
  // mirror can pass (0.6, 0.7, 0.8) or similar.
  void setMirrorOverlay(bool enabled, float intensity = 0.5f,
                        const glm::vec3 &tint = glm::vec3(1.0f)) {
    hasMirrorOverlay_ = enabled;
    mirrorIntensity_ = intensity;
    mirrorTint_ = tint;
  }
  bool hasMirrorOverlay() const { return hasMirrorOverlay_; }
  float getMirrorIntensity() const { return mirrorIntensity_; }
  glm::vec3 getMirrorTint() const { return mirrorTint_; }

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

  std::shared_ptr<Portal> destination_{nullptr};   // Doorway-based destination (self == mirror)
  std::shared_ptr<render::PortalFramebuffer> framebuffer_{nullptr};

  bool visible_{true};
  bool enabled_{true};
  bool isOpen_{true};           // Is the doorway open? (for doors)
  bool isPassable_{true};       // Can entities walk through this portal?
  bool hasMirrorOverlay_{false}; // Optional mirror effect overlay
  float mirrorIntensity_{0.5f};  // Mirror overlay intensity (0.0 = transparent, 1.0 = full mirror)
  // Per-portal colour applied to the destination view when the overlay is
  // on. Defaults to white so enabling the overlay without setting a tint is
  // a no-op; see setMirrorOverlay().
  glm::vec3 mirrorTint_{1.0f, 1.0f, 1.0f};
};

}  // namespace geometry
}  // namespace omega

