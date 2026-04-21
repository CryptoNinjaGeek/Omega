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

  // --- Phase 2.2 public surface ------------------------------------------
  // After the PortalPair retirement there are four supported entry points:
  //   * calculatePortalViewUnified — view matrix for rendering through a portal
  //   * getClippingPlane           — oblique-clip plane for the destination
  //   * isPortalVisible            — front-facing + distance check
  //   * isInViewFrustum            — frustum rejection on the quad corners
  //
  // `transformPosition`/`transformDirection` are intentionally kept public
  // even though they're not strictly part of the render pipeline: Phase 4
  // (entity traversal) will call them to teleport the player/AI through a
  // portal, and `PortalCameraTest` exercises them today. Everything else
  // (doorway/mirror dispatchers, relative-transform helper) is private.

  /**
   * Unified interface for calculating portal view. Looks at
   * `portal.getDestination()` and picks mirror vs doorway math automatically.
   * This is the only view-matrix entry point renderer code should use.
   * @param playerCamera The player's current camera
   * @param portal The portal to look through
   * @return View matrix for portal rendering
   */
  static glm::mat4 calculatePortalViewUnified(
      const Camera& playerCamera,
      const geometry::Portal& portal);

  /**
   * Get clipping plane for portal (prevents seeing portal backside).
   * @return Clipping plane equation (normal.x, normal.y, normal.z, distance)
   */
  static glm::vec4 getClippingPlane(const geometry::Portal& portal);

  /**
   * Check if portal is visible from camera — front-facing plus max-distance
   * LOD cut.
   */
  static bool isPortalVisible(const geometry::Portal& portal, const Camera& camera);

  /**
   * Check if portal is in camera's view frustum (all four corners tested).
   */
  static bool isInViewFrustum(const geometry::Portal& portal, const Camera& camera);

  /**
   * Transform a position from source portal space to destination portal
   * space. Entry point for Phase 4 entity traversal (teleporting players /
   * AI through a portal pair); exercised today by `PortalCameraTest`.
   */
  static glm::vec3 transformPosition(
      const glm::vec3& position,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal);

  /**
   * Transform a direction (no translation) through a portal pair. Pair to
   * `transformPosition`; used to rotate velocities / facing when an entity
   * passes through.
   */
  static glm::vec3 transformDirection(
      const glm::vec3& direction,
      const geometry::Portal& sourcePortal,
      const geometry::Portal& destinationPortal);

private:
  /**
   * Calculate view matrix for doorway portal (destination != self). Uses
   * Valve-style relative transform with a 180° yaw about local up to flip
   * the player's camera through the pair. Dispatched internally by
   * `calculatePortalViewUnified` — callers should not invoke directly.
   */
  static glm::mat4 calculateDoorwayView(
      const Camera& playerCamera,
      const geometry::Portal& portal);

  /**
   * Calculate view matrix for mirror portal (destination == self). Reflects
   * the player camera across the portal plane. Dispatched internally by
   * `calculatePortalViewUnified`.
   */
  static glm::mat4 calculateMirrorView(
      const Camera& playerCamera,
      const geometry::Portal& portal);

  /**
   * Calculate relative transform between two portals (M_rel = T_dst *
   * inverse(T_src)). Used by `transformPosition`/`transformDirection`.
   */
  static glm::mat4 calculateRelativeTransform(
      const geometry::Portal& source,
      const geometry::Portal& destination);
};

}  // namespace render
}  // namespace omega

