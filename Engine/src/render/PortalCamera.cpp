#include <render/PortalCamera.h>
#include <render/Camera.h>
#include <render/Frustum.h>
#include <geometry/Portal.h>
#include <system/Log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace omega::render;
using namespace omega::geometry;

glm::mat4 PortalCamera::calculatePortalView(
    const Camera& playerCamera,
    const Portal& sourcePortal,
    const Portal& destinationPortal) {
  // Transform the entire camera transform matrix through the portal
  // This preserves all orientation including pitch correctly
  
  // Get player camera's view matrix and convert to world transform
  glm::mat4 playerView = playerCamera.viewMatrix();
  glm::mat4 playerWorld = glm::inverse(playerView);
  
  // Get portal transforms
  glm::mat4 sourceTransform = sourcePortal.getTransform();
  glm::mat4 destTransform = destinationPortal.getTransform();
  
  // Calculate relative transform: transforms from source portal space to destination portal space
  glm::mat4 relativeTransform = destTransform * glm::inverse(sourceTransform);
  
  // Transform camera position through portal correctly
  // The key is to preserve the relative position and height to the portal
  glm::vec3 playerPos = playerCamera.position();
  
  // Get relative position to source portal (preserves height and offset)
  glm::vec3 relativeToSource = playerPos - sourcePortal.getPosition();
  
  // Transform this relative position through the portal rotation
  // First, express in source portal's local coordinate system
  glm::mat3 sourceRotation = glm::mat3(sourceTransform);
  glm::vec3 localPos = glm::inverse(sourceRotation) * relativeToSource;
  
  // Transform to destination portal's local coordinate system
  glm::mat3 destRotation = glm::mat3(destTransform);
  glm::vec3 localPosInDest = destRotation * localPos;
  
  // Convert back to world space (preserves relative height and position)
  glm::vec3 transformedPos = localPosInDest + destinationPortal.getPosition();
  
  // Transform the camera's world transform through the portal
  // Express camera transform relative to source portal (rotation only, position handled separately)
  glm::mat4 sourceRotationOnly = sourceTransform;
  sourceRotationOnly[3][0] = 0.0f;
  sourceRotationOnly[3][1] = 0.0f;
  sourceRotationOnly[3][2] = 0.0f;
  sourceRotationOnly[3][3] = 1.0f;
  
  glm::mat4 destRotationOnly = destTransform;
  destRotationOnly[3][0] = 0.0f;
  destRotationOnly[3][1] = 0.0f;
  destRotationOnly[3][2] = 0.0f;
  destRotationOnly[3][3] = 1.0f;
  
  // Transform camera orientation (not position) through portal
  glm::mat4 cameraRotationInSource = glm::inverse(sourceRotationOnly) * playerWorld;
  cameraRotationInSource[3][0] = 0.0f;
  cameraRotationInSource[3][1] = 0.0f;
  cameraRotationInSource[3][2] = 0.0f;
  cameraRotationInSource[3][3] = 1.0f;
  
  glm::mat4 transformedRotation = destRotationOnly * cameraRotationInSource;
  
  // Build final world matrix with correct position and orientation
  glm::mat4 transformedWorld = transformedRotation;
  transformedWorld[3][0] = transformedPos.x;
  transformedWorld[3][1] = transformedPos.y;
  transformedWorld[3][2] = transformedPos.z;
  transformedWorld[3][3] = 1.0f;
  
  // Convert back to view matrix (view = inverse(world))
  glm::mat4 portalView = glm::inverse(transformedWorld);
  
  // Trace-level log so it's opt-in via OMEGA_LOG_LEVEL=trace.
  {
    const glm::vec3 transformedUp = glm::normalize(
        glm::vec3(portalView[0][1], portalView[1][1], portalView[2][1]));
    const glm::vec3 playerUp = glm::normalize(
        glm::vec3(playerView[0][1], playerView[1][1], playerView[2][1]));
    const glm::vec3 sourceUp = sourcePortal.getUp();
    const glm::vec3 destUp = destinationPortal.getUp();
    OMEGA_LOG_TRACE("portal-cam",
                    "calcPortalView: playerUp=({},{},{}) srcUp=({},{},{}) "
                    "dstUp=({},{},{}) transformedUp=({},{},{})",
                    playerUp.x, playerUp.y, playerUp.z, sourceUp.x, sourceUp.y,
                    sourceUp.z, destUp.x, destUp.y, destUp.z, transformedUp.x,
                    transformedUp.y, transformedUp.z);
  }

  return portalView;
}

glm::mat4 PortalCamera::calculatePortalViewWithClipping(
    const Camera& playerCamera,
    const Portal& sourcePortal,
    const Portal& destinationPortal,
    glm::vec4& clippingPlane) {
  // Calculate clipping plane in destination portal space
  clippingPlane = getClippingPlane(destinationPortal);

  // Calculate portal view
  return calculatePortalView(playerCamera, sourcePortal, destinationPortal);
}

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
  // Get the destination portal
  auto dest = portal.getDestination();
  if (!dest) {
    // Fallback: use linked portal for backward compatibility
    dest = portal.getLinkedPortal();
    if (!dest) {
      // No destination, return identity (shouldn't happen)
      return playerCamera.viewMatrix();
    }
  }
  
  // Use existing calculatePortalView method (it already handles doorway transformation)
  return calculatePortalView(playerCamera, portal, *dest);
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

