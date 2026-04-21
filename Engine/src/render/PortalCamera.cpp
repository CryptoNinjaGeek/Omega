#include <render/PortalCamera.h>
#include <render/Camera.h>
#include <render/Frustum.h>
#include <geometry/Portal.h>
#include <system/Log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace omega::render;
using namespace omega::geometry;

glm::vec3 PortalCamera::transformPosition(
    const glm::vec3& position,
    const Portal& sourcePortal,
    const Portal& destinationPortal) {
  // Transform position from source portal space to destination portal space
  glm::mat4 relativeTransform = calculateRelativeTransform(sourcePortal, destinationPortal);
  
  glm::vec3 localPos = position - sourcePortal.getPosition();
  glm::vec4 localPos4 = glm::vec4(localPos, 1.0f);
  localPos4 = relativeTransform * localPos4;
  
  return glm::vec3(localPos4) + destinationPortal.getPosition();
}

glm::vec3 PortalCamera::transformDirection(
    const glm::vec3& direction,
    const Portal& sourcePortal,
    const Portal& destinationPortal) {
  // Transform direction vector (no translation, only rotation)
  glm::mat4 relativeTransform = calculateRelativeTransform(sourcePortal, destinationPortal);
  
  // Extract rotation part (remove translation)
  glm::mat3 rotation = glm::mat3(relativeTransform);
  
  return rotation * direction;
}

glm::vec4 PortalCamera::getClippingPlane(const Portal& portal) {
  // Clipping plane: normal points away from portal (opposite of portal normal)
  // This prevents seeing through the back of the portal
  glm::vec3 normal = -portal.getNormal();  // Negative normal (points away from portal)
  float distance = glm::dot(normal, portal.getPosition());
  
  return glm::vec4(normal, distance);
}

glm::mat4 PortalCamera::calculateRelativeTransform(
    const Portal& source,
    const Portal& destination) {
  // Calculate transform from source portal space to destination portal space
  glm::mat4 sourceTransform = source.getTransform();
  glm::mat4 destTransform = destination.getTransform();
  
  // Relative transform: destination * inverse(source)
  return destTransform * glm::inverse(sourceTransform);
}

glm::mat4 PortalCamera::calculateDoorwayView(
    const Camera& playerCamera,
    const Portal& portal) {
  auto dest = portal.getDestination();
  if (!dest) {
    // No destination wired up — nothing sensible to render. Return the
    // player's own view so the FBO is at least a plausible image rather
    // than uninitialised garbage.
    return playerCamera.viewMatrix();
  }

  // Standard paired-portal ("Portal"/"Prey"-style) camera transform.
  //
  // Given the player's world transform W_P and the two portals' local-to-world
  // transforms T_src and T_dst, the virtual camera's world transform is:
  //
  //   W_V    = M_rel * W_P
  //   M_rel  = T_dst * R_180y * inverse(T_src)
  //
  // The R_180y factor (180° rotation about the portal's LOCAL +Y / up axis) is
  // what makes a "doorway" pair actually behave like a doorway: entering the
  // front face of the source portal corresponds to exiting the front face of
  // the destination portal. In local coordinates, that means source-local +Z
  // ("into the source portal") must map to destination-local -Z ("out of the
  // destination portal"), which is a 180° turn about local up.
  //
  // Dropping this factor degenerates the transform into a mirror-like
  // mapping that renders the same room from a warped, incorrect vantage —
  // which was the prior bug (Phase 1.2b).
  const glm::mat4 T_src = portal.getTransform();
  const glm::mat4 T_dst = dest->getTransform();

  const glm::mat4 R_180y =
      glm::rotate(glm::mat4(1.0f), glm::pi<float>(),
                  glm::vec3(0.0f, 1.0f, 0.0f));

  const glm::mat4 M_rel = T_dst * R_180y * glm::inverse(T_src);

  const glm::mat4 playerView  = playerCamera.viewMatrix();
  const glm::mat4 playerWorld = glm::inverse(playerView);
  const glm::mat4 virtualWorld = M_rel * playerWorld;

  // Trace-level log so it's opt-in via OMEGA_LOG_LEVEL=trace.
  {
    const glm::vec3 playerPos(playerWorld[3]);
    const glm::vec3 virtPos(virtualWorld[3]);
    const glm::vec3 playerUp = glm::normalize(glm::vec3(playerWorld[1]));
    const glm::vec3 virtUp   = glm::normalize(glm::vec3(virtualWorld[1]));
    const glm::vec3 sourceUp = portal.getUp();
    const glm::vec3 destUp   = dest->getUp();
    OMEGA_LOG_TRACE("portal-cam",
                    "calcDoorwayView: playerPos=({},{},{}) virtPos=({},{},{}) "
                    "srcUp=({},{},{}) dstUp=({},{},{}) "
                    "playerUp=({},{},{}) virtUp=({},{},{})",
                    playerPos.x, playerPos.y, playerPos.z,
                    virtPos.x, virtPos.y, virtPos.z,
                    sourceUp.x, sourceUp.y, sourceUp.z,
                    destUp.x, destUp.y, destUp.z,
                    playerUp.x, playerUp.y, playerUp.z,
                    virtUp.x, virtUp.y, virtUp.z);
  }

  return glm::inverse(virtualWorld);
}

glm::mat4 PortalCamera::calculateMirrorView(
    const Camera& playerCamera,
    const Portal& portal) {
  // Mirror: reflect camera through portal plane
  glm::mat4 portalTransform = portal.getTransform();
  
  // Get portal normal and position for reflection
  glm::vec3 portalNormal = portal.getNormal();
  glm::vec3 portalPos = portal.getPosition();
  
  // Create reflection matrix
  // Reflection matrix: I - 2 * n * n^T (where n is the normal)
  glm::mat4 reflection = glm::mat4(1.0f);
  reflection[0][0] = 1.0f - 2.0f * portalNormal.x * portalNormal.x;
  reflection[0][1] = -2.0f * portalNormal.x * portalNormal.y;
  reflection[0][2] = -2.0f * portalNormal.x * portalNormal.z;
  
  reflection[1][0] = -2.0f * portalNormal.y * portalNormal.x;
  reflection[1][1] = 1.0f - 2.0f * portalNormal.y * portalNormal.y;
  reflection[1][2] = -2.0f * portalNormal.y * portalNormal.z;
  
  reflection[2][0] = -2.0f * portalNormal.z * portalNormal.x;
  reflection[2][1] = -2.0f * portalNormal.z * portalNormal.y;
  reflection[2][2] = 1.0f - 2.0f * portalNormal.z * portalNormal.z;
  
  // Transform player camera world matrix
  glm::mat4 playerView = playerCamera.viewMatrix();
  glm::mat4 playerWorld = glm::inverse(playerView);
  
  // Reflect the world matrix
  glm::mat4 mirroredWorld = reflection * playerWorld;
  
  // Reflect position
  glm::vec3 playerPos = playerCamera.position();
  glm::vec3 toPortal = playerPos - portalPos;
  float distance = glm::dot(toPortal, portalNormal);
  glm::vec3 mirroredPos = playerPos - 2.0f * distance * portalNormal;
  
  // Update position in world matrix
  mirroredWorld[3][0] = mirroredPos.x;
  mirroredWorld[3][1] = mirroredPos.y;
  mirroredWorld[3][2] = mirroredPos.z;
  mirroredWorld[3][3] = 1.0f;
  
  // Convert back to view matrix
  return glm::inverse(mirroredWorld);
}

glm::mat4 PortalCamera::calculatePortalViewUnified(
    const Camera& playerCamera,
    const Portal& portal) {
  // Automatically detect if portal is a mirror or doorway
  if (portal.isMirror()) {
    return calculateMirrorView(playerCamera, portal);
  } else {
    return calculateDoorwayView(playerCamera, portal);
  }
}

bool PortalCamera::isPortalVisible(const Portal& portal, const Camera& camera) {
  // Check if portal is facing camera
  glm::vec3 toPortal = portal.getPosition() - camera.position();
  float distance = glm::length(toPortal);
  
  if (distance < 0.001f) {
    return false;  // Too close or at same position
  }
  
  glm::vec3 toPortalNormalized = glm::normalize(toPortal);
  float dot = glm::dot(toPortalNormalized, portal.getNormal());
  
  // Portal is visible if camera is in front of it (dot < 0 means portal normal points away from camera)
  // Actually, we want portal facing camera, so dot should be negative
  if (dot > 0.0f) {
    return false;  // Portal is facing away from camera
  }
  
  // Optional: Check distance (LOD)
  const float MAX_PORTAL_VIEW_DISTANCE = 100.0f;
  if (distance > MAX_PORTAL_VIEW_DISTANCE) {
    return false;
  }
  
  return true;
}

bool PortalCamera::isInViewFrustum(const Portal& portal, const Camera& camera) {
  // Extract the six world-space frustum planes from projection * view (see
  // Frustum::extractPlanes for the Gribb–Hartmann derivation) and test the
  // portal's four world-space corners against them.
  //
  // We use Frustum::allPointsOutsideAnyPlane as the rejection test: the
  // portal is culled iff all four corners lie on the outside of the same
  // plane. This is the standard conservative polygon-frustum test — it can
  // occasionally mis-cull large portals that straddle a corner of the
  // frustum (no single plane rejects all four corners), but for portal
  // quads those cases are rare and false positives only cost an extra FBO
  // render, not correctness.
  const glm::mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();
  const auto planes = Frustum::extractPlanes(viewProj);

  glm::vec3 corners[4];
  portal.getCorners(corners);

  return !Frustum::allPointsOutsideAnyPlane(planes, corners, 4);
}

