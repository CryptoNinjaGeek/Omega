#pragma once

#include <system/Global.h>
#include <render/Camera.h>
#include <geometry/Portal.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace omega {
namespace render {

/**
 * PortalCamera - Extends Camera for portal-specific view calculations
 * Calculates view matrices for rendering through portals
 */
class OMEGA_EXPORT PortalCamera {
public:
  PortalCamera() = default;

  /**
   * Calculate view matrix for rendering through a portal
   * @param playerCamera The player's current camera
   * @param sourcePortal The portal the player is looking through
   * @param destinationPortal The portal that shows the destination view
   * @return View matrix for portal rendering
   */
  static glm::mat4 calculatePortalView(
      const Camera& playerCamera,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal);

  /**
   * Calculate view matrix with clipping plane support
   * Prevents seeing through the back of the portal
   */
  static glm::mat4 calculatePortalViewWithClipping(
      const Camera& playerCamera,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal,
      glm::vec4& clippingPlane);

  /**
   * Transform a position through a portal pair
   * @param position Position in source portal space
   * @param sourcePortal Source portal
   * @param destinationPortal Destination portal
   * @return Transformed position in destination portal space
   */
  static glm::vec3 transformPosition(
      const glm::vec3& position,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal);

  /**
   * Transform a direction through a portal pair
   * @param direction Direction vector in source portal space
   * @param sourcePortal Source portal
   * @param destinationPortal Destination portal
   * @return Transformed direction in destination portal space
   */
  static glm::vec3 transformDirection(
      const glm::vec3& direction,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal);

  /**
   * Get clipping plane for portal (prevents seeing portal backside)
   * @param portal The portal to get clipping plane for
   * @return Clipping plane equation (normal.x, normal.y, normal.z, distance)
   */
  static glm::vec4 getClippingPlane(const geometry::Portal& portal);

private:
  /**
   * Calculate relative transform between two portals
   */
  static glm::mat4 calculateRelativeTransform(
      const geometry::Portal& source,
      const geometry::Portal& destination);
};

}  // namespace render
}  // namespace omega

