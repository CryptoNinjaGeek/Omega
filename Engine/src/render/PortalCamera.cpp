#include <render/PortalCamera.h>
#include <render/Camera.h>
#include <geometry/Portal.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

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
  
  // Debug: Extract and verify the transformed camera's up vector
  static int debugCount = 0;
  if (debugCount++ < 5) {
    glm::vec3 transformedUp = glm::normalize(glm::vec3(portalView[0][1], portalView[1][1], portalView[2][1]));
    glm::vec3 playerUp = glm::normalize(glm::vec3(playerView[0][1], playerView[1][1], playerView[2][1]));
    glm::vec3 sourceUp = sourcePortal.getUp();
    glm::vec3 destUp = destinationPortal.getUp();
    
    std::cout << "[PortalCamera] Debug call #" << debugCount << ":" << std::endl;
    std::cout << "  Player up: (" << playerUp.x << ", " << playerUp.y << ", " << playerUp.z << ")" << std::endl;
    std::cout << "  Source portal up: (" << sourceUp.x << ", " << sourceUp.y << ", " << sourceUp.z << ")" << std::endl;
    std::cout << "  Dest portal up: (" << destUp.x << ", " << destUp.y << ", " << destUp.z << ")" << std::endl;
    std::cout << "  Transformed up: (" << transformedUp.x << ", " << transformedUp.y << ", " << transformedUp.z << ")" << std::endl;
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

