#include <geometry/PortalRenderer.h>
#include <geometry/Portal.h>
#include <geometry/PortalPair.h>
#include <geometry/PortalSurface.h>
#include <geometry/Scene.h>
#include <render/Camera.h>
#include <render/PortalFramebuffer.h>
#include <render/PortalCamera.h>
#include <render/PortalViewCamera.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <system/Log.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace omega::geometry;
using namespace omega::render;

PortalRenderer::PortalRenderer() = default;

std::shared_ptr<Shader> PortalRenderer::ensurePortalShader() {
  if (portalShader_ && portalShader_->isValid()) {
    return portalShader_;
  }

  // Try paths in priority order. The first successful *and valid* load wins.
  // Priority: resource zip (preferred, ships with the app) → disk relative to
  // CWD (dev convenience when running from bin/) → resources/ subdir (older
  // layout still used by some demos).
  //
  // Shader::fromFile always returns a non-null shared_ptr even on failure,
  // so we must check isValid() (which verifies GL_LINK_STATUS) rather than
  // just the pointer.
  static const std::pair<const char *, const char *> kCandidates[] = {
      {":/shaders/portal.vs", ":/shaders/portal.fs"},
      {"portal.vs", "portal.fs"},
      {"resources/shaders/portal.vs", "resources/shaders/portal.fs"},
  };

  for (const auto &[vs, fs] : kCandidates) {
    std::shared_ptr<Shader> candidate;
    try {
      candidate = Shader::fromFile(3, 3, vs, fs);
    } catch (const std::exception &e) {
      OMEGA_LOG_WARN("portal", "Portal shader load threw for {}+{}: {}", vs,
                     fs, e.what());
      continue;
    }
    if (candidate && candidate->isValid()) {
      OMEGA_LOG_INFO("portal", "Loaded portal shader from {} + {}", vs, fs);
      portalShader_ = candidate;
      return portalShader_;
    }
  }

  OMEGA_LOG_ERROR(
      "portal",
      "Portal shader could not be loaded from any candidate path "
      "(tried :/shaders/portal.{{vs,fs}}, portal.{{vs,fs}}, "
      "resources/shaders/portal.{{vs,fs}})");
  portalShader_.reset();
  return portalShader_;
}

void PortalRenderer::addPortalPair(std::shared_ptr<PortalPair> portalPair) {
  if (portalPair && portalPair->isValid()) {
    portalPairs_.push_back(portalPair);
  }
}

void PortalRenderer::addPortal(std::shared_ptr<Portal> portal) {
  if (portal) {
    portals_.push_back(portal);
  }
}

void PortalRenderer::clearPortals() {
  portalPairs_.clear();
  portals_.clear();
  activePortals_.clear();
}

void PortalRenderer::renderPortals(std::shared_ptr<Scene> scene,
                                   std::shared_ptr<Camera> playerCamera,
                                   std::shared_ptr<Shader> portalShader) {
  if (!enabled_ || !scene || !playerCamera) {
    return;
  }

  // Clear active portals tracking for this frame
  activePortals_.clear();

  // Phase 1: Render portal views (back rendering) - BEFORE main scene
  // This happens BEFORE the main scene so we can use the framebuffers when rendering surfaces

  // Legacy: Render PortalPairs (for backward compatibility)
  for (auto& portalPair : portalPairs_) {
    if (!portalPair->isEnabled()) {
      continue;
    }

    auto portalA = portalPair->getPortalA();
    auto portalB = portalPair->getPortalB();

    if (!portalA || !portalB) {
      continue;
    }

    // Render portal A's view (what you see through portal A)
    if (shouldRenderPortal(portalA, playerCamera, 0)) {
      renderPortalView(portalA, portalB, scene, playerCamera, 0);
    }

    // Render portal B's view (what you see through portal B)
    if (shouldRenderPortal(portalB, playerCamera, 0)) {
      renderPortalView(portalB, portalA, scene, playerCamera, 0);
    }
  }

  // New: Render standalone portals (doorway-based system)
  for (auto& portal : portals_) {
    if (shouldRenderPortal(portal, playerCamera, 0)) {
      renderPortalViewRecursive(portal, scene, playerCamera, 0);
    }
  }
  
  // Ensure viewport is restored to window size after portal view rendering
  GLFWwindow* currentWindow = glfwGetCurrentContext();
  if (currentWindow) {
    int width, height;
    glfwGetFramebufferSize(currentWindow, &width, &height);
    glViewport(0, 0, width, height);
  }
}

void PortalRenderer::renderPortalSurfaces(std::shared_ptr<Camera> playerCamera,
                                          std::shared_ptr<Shader> portalShader) {
  if (!enabled_ || !playerCamera) {
    return;
  }

  // Phase 2: Render portal surfaces (forward rendering) - AFTER main scene
  // Render portal surfaces AFTER the main scene so they appear on top
  
  // Legacy: Render PortalPair surfaces
  for (auto& portalPair : portalPairs_) {
    if (!portalPair->isEnabled()) {
      continue;
    }

    auto portalA = portalPair->getPortalA();
    auto portalB = portalPair->getPortalB();

    if (portalA && portalA->isVisible() && portalA->isEnabled()) {
      renderPortalSurface(portalA, playerCamera, portalShader);
    }

    if (portalB && portalB->isVisible() && portalB->isEnabled()) {
      renderPortalSurface(portalB, playerCamera, portalShader);
    }
  }

  // New: Render standalone portal surfaces
  for (auto& portal : portals_) {
    if (portal && portal->isVisible() && portal->isEnabled()) {
      renderPortalSurface(portal, playerCamera, portalShader);
    }
  }
}

void PortalRenderer::renderPortalView(std::shared_ptr<Portal> sourcePortal,
                                     std::shared_ptr<Portal> destPortal,
                                     std::shared_ptr<Scene> scene,
                                     std::shared_ptr<Camera> playerCamera,
                                     int recursionDepth) {
  if (!sourcePortal || !destPortal || !scene || !playerCamera) {
    return;
  }

  // Prevent infinite recursion
  if (recursionDepth >= maxRecursionDepth_) {
    return;
  }

  auto framebuffer = sourcePortal->getFramebuffer();
  if (!framebuffer || !framebuffer->isComplete()) {
    return;
  }

  // Bind portal framebuffer
  framebuffer->bind();
  // Clear to a visible color to test framebuffer rendering
  // Use a blue-green color so we can see if framebuffer is working
  framebuffer->clear(0.2f, 0.5f, 0.8f, 1.0f);  // Blue-green background

  // Calculate portal camera view matrix
  // Use unified method if source portal has destination set, otherwise use legacy method
  glm::mat4 portalView;
  if (sourcePortal->getDestination()) {
    portalView = PortalCamera::calculatePortalViewUnified(*playerCamera, *sourcePortal);
  } else {
    portalView = PortalCamera::calculatePortalView(
        *playerCamera, *sourcePortal, *destPortal);
  }

  // Create temporary camera with portal view
  auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);

  // Mirror the player camera's perspective onto the portal camera. We reuse
  // the exact FOV/near/far the player is using (via the new accessors added
  // in Phase 0.2) so that geometry seen through a portal matches the player's
  // lens; only the aspect ratio is remapped to the framebuffer's dimensions.
  portalCamera->setPerspective(
      playerCamera->fov(),
      static_cast<float>(framebuffer->getWidth()),
      static_cast<float>(framebuffer->getHeight()),
      playerCamera->nearPlane(),
      playerCamera->farPlane());

  // Render scene from portal perspective.
  // Note: This renders scene objects only. Nested portal recursion is handled
  // by renderPortalViewRecursive calling back into renderPortalView.
  {
    const glm::vec3 camPos = portalCamera->position();
    const glm::vec3 playerPos = playerCamera->position();
    const glm::vec3 sourcePos = sourcePortal->getPosition();
    const glm::vec3 destPos = destPortal->getPosition();
    OMEGA_LOG_TRACE("portal",
                    "renderPortalView: player=({},{},{}) src=({},{},{}) "
                    "dst=({},{},{}) portalCam=({},{},{}) fb={}x{}",
                    playerPos.x, playerPos.y, playerPos.z, sourcePos.x,
                    sourcePos.y, sourcePos.z, destPos.x, destPos.y, destPos.z,
                    camPos.x, camPos.y, camPos.z, framebuffer->getWidth(),
                    framebuffer->getHeight());
  }

  OMEGA_GL_CHECK("portal/renderPortalView: before scene render");

  GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
    OMEGA_LOG_ERROR("portal",
                    "Framebuffer not complete! Status: 0x{:x}",
                    static_cast<unsigned>(fbStatus));
  }

  // Enable oblique clipping against the destination portal's plane so that
  // geometry on the *camera* side of the destination (i.e. between the
  // portal camera and the destination surface) is discarded — only the
  // half-space "behind" the destination portal, the room we're peering into,
  // should fill the FBO. See core.vs for the exact equation and
  // Portal::getClippingPlane for the encoding.
  //
  // Scene::render() toggles GL_CLIP_DISTANCE0 based on this state and pushes
  // (clippingPlane, enableClipping) into every object's shader as it walks
  // the tree.
  const glm::vec4 destPlane = destPortal->getClippingPlane();
  scene->setActiveClippingPlane(destPlane, true);
  scene->render(portalCamera);
  scene->setActiveClippingPlane(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), false);

  OMEGA_GL_CHECK("portal/renderPortalView: after scene render");

  framebuffer->unbind();
}

void PortalRenderer::renderPortalSurface(std::shared_ptr<Portal> portal,
                                        std::shared_ptr<Camera> playerCamera,
                                        std::shared_ptr<Shader> portalShader) {
  if (!portal || !playerCamera) {
    return;
  }

  auto framebuffer = portal->getFramebuffer();
  if (!framebuffer || !framebuffer->isComplete()) {
    return;
  }

  // Use the portal shader owned by the renderer (lazy-loaded on first use).
  // The passed-in portalShader argument is ignored — the renderer owns its own
  // shader so consumers don't need to load it themselves.
  if (!ensurePortalShader()) {
    return;  // error already logged by ensurePortalShader()
  }
  portalShader = portalShader_;

  // Get or create portal surface object
  std::shared_ptr<Object> portalSurface;
  auto it = portalSurfaces_.find(portal);
  if (it == portalSurfaces_.end()) {
    // Create portal surface mesh
    portalSurface = PortalSurface::createSurface(portal, portalShader);
    if (portalSurface) {
      portalSurfaces_[portal] = portalSurface;
    } else {
      return;
    }
  } else {
    portalSurface = it->second;
  }

  if (!portalSurface) {
    return;
  }

  // Render portal surface directly (bypass Object::render() which expects lighting shader)
  // Get the portal surface's VAO and count
  unsigned int vao = portalSurface->getVAO();
  unsigned int count = portalSurface->getCount();
  ObjectType type = portalSurface->getType();
  
  OMEGA_LOG_TRACE("portal",
                  "renderPortalSurface: vao={} count={} texId={} pos=({},{},{})",
                  vao, count, framebuffer->getColorTexture(),
                  portal->getPosition().x, portal->getPosition().y,
                  portal->getPosition().z);

  if (vao == 0 || count == 0) {
    OMEGA_LOG_ERROR("portal", "Invalid VAO={} or count={}", vao, count);
    return;
  }

  const unsigned int textureId = framebuffer->getColorTexture();
  if (textureId == 0) {
    OMEGA_LOG_ERROR("portal", "Framebuffer texture ID is 0");
    return;
  }

  portalShader->use();
  OMEGA_GL_CHECK("portal/renderPortalSurface: after shader use");

  GLint currentProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  if (currentProgram == 0) {
    OMEGA_LOG_ERROR("portal", "No shader program active after use()");
    return;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureId);
  const GLint texLoc = glGetUniformLocation(currentProgram, "portalTexture");
  if (texLoc >= 0) {
    glUniform1i(texLoc, 0);
  } else {
    OMEGA_LOG_WARN("portal", "portalTexture uniform not found in shader");
  }

  const glm::mat4 projection = playerCamera->projectionMatrix();
  const glm::mat4 view = playerCamera->viewMatrix();
  const glm::mat4 model = portalSurface->getModel();

  const GLint projLoc = glGetUniformLocation(currentProgram, "projection");
  const GLint viewLoc = glGetUniformLocation(currentProgram, "view");
  const GLint modelLoc = glGetUniformLocation(currentProgram, "model");

  if (projLoc >= 0) {
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
  } else {
    OMEGA_LOG_WARN("portal", "projection uniform not found");
  }
  if (viewLoc >= 0) {
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
  } else {
    OMEGA_LOG_WARN("portal", "view uniform not found");
  }
  if (modelLoc >= 0) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  } else {
    OMEGA_LOG_WARN("portal", "model uniform not found");
  }

  OMEGA_GL_CHECK("portal/renderPortalSurface: after setting uniforms");

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  glBindVertexArray(vao);
  OMEGA_GL_CHECK("portal/renderPortalSurface: after bind VAO");

  if (type == ObjectType::Elements) {
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, count);
  }
  glBindVertexArray(0);
  OMEGA_GL_CHECK("portal/renderPortalSurface: after draw");

  glBindTexture(GL_TEXTURE_2D, 0);
}

bool PortalRenderer::isPortalVisible(std::shared_ptr<Portal> portal,
                                    std::shared_ptr<Camera> camera) const {
  if (!portal || !camera) {
    return false;
  }

  // Use PortalCamera's visibility check (includes facing check and distance)
  return PortalCamera::isPortalVisible(*portal, *camera);
}

bool PortalRenderer::shouldRenderPortal(std::shared_ptr<Portal> portal,
                                       std::shared_ptr<Camera> camera,
                                       int recursionDepth) const {
  if (!portal || !camera) {
    return false;
  }

  // Check if portal is closed
  if (!portal->isOpen()) {
    return false;
  }

  // Check if portal is enabled
  if (!portal->isEnabled()) {
    return false;
  }

  // Check if portal is visible from camera
  if (!PortalCamera::isPortalVisible(*portal, *camera)) {
    return false;
  }

  // Check if portal is in view frustum
  if (!PortalCamera::isInViewFrustum(*portal, *camera)) {
    return false;
  }

  // Check recursion limit
  if (recursionDepth >= maxRecursionDepth_) {
    return false;
  }

  // Check for infinite loops (portal already being rendered)
  if (activePortals_.count(portal) > 0) {
    return false;
  }

  return true;
}

void PortalRenderer::renderPortalViewRecursive(std::shared_ptr<Portal> portal,
                                              std::shared_ptr<Scene> scene,
                                              std::shared_ptr<Camera> playerCamera,
                                              int recursionDepth) {
  if (!portal || !scene || !playerCamera) {
    return;
  }

  // Mark portal as being rendered
  activePortals_.insert(portal);

  // Get destination portal
  auto dest = portal->getDestination();
  if (!dest) {
    // Fallback: use linked portal for backward compatibility
    dest = portal->getLinkedPortal();
    if (!dest) {
      activePortals_.erase(portal);
      return;  // No destination
    }
  }

  // Render portal view
  renderPortalView(portal, dest, scene, playerCamera, recursionDepth);

  // Recursively render nested portals visible through this portal
  // Check all portals in the scene (both PortalPairs and standalone)
  for (auto& nestedPair : portalPairs_) {
    if (!nestedPair->isEnabled()) {
      continue;
    }
    auto nestedA = nestedPair->getPortalA();
    auto nestedB = nestedPair->getPortalB();
    
    // Skip source and destination portals to avoid immediate loops
    if (nestedA == portal || nestedA == dest || nestedB == portal || nestedB == dest) {
      continue;
    }

    // Create a temporary portal camera for visibility checks
    glm::mat4 portalView = PortalCamera::calculatePortalViewUnified(*playerCamera, *portal);
    auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);

    // Check nested portals
    if (nestedA && shouldRenderPortal(nestedA, portalCamera, recursionDepth + 1)) {
      renderPortalViewRecursive(nestedA, scene, portalCamera, recursionDepth + 1);
    }
    if (nestedB && shouldRenderPortal(nestedB, portalCamera, recursionDepth + 1)) {
      renderPortalViewRecursive(nestedB, scene, portalCamera, recursionDepth + 1);
    }
  }

  // Check standalone portals
  for (auto& nestedPortal : portals_) {
    // Skip source and destination portals
    if (nestedPortal == portal || nestedPortal == dest) {
      continue;
    }

    // Create a temporary portal camera for visibility checks
    glm::mat4 portalView = PortalCamera::calculatePortalViewUnified(*playerCamera, *portal);
    auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);

    if (shouldRenderPortal(nestedPortal, portalCamera, recursionDepth + 1)) {
      renderPortalViewRecursive(nestedPortal, scene, portalCamera, recursionDepth + 1);
    }
  }

  // Unmark portal
  activePortals_.erase(portal);
}

