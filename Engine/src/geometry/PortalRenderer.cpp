#include <geometry/PortalRenderer.h>
#include <geometry/Portal.h>
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

void PortalRenderer::addPortal(std::shared_ptr<Portal> portal) {
  if (portal) {
    portals_.push_back(portal);
  }
}

void PortalRenderer::clearPortals() {
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
  // This happens BEFORE the main scene so we can use the framebuffers when
  // rendering surfaces. Each portal carries its own `destination_`; mirrors
  // (dest == self) and doorways are both handled by `renderPortalViewRecursive`
  // via `PortalCamera::calculatePortalViewUnified`.
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
  // Render portal surfaces AFTER the main scene so they appear on top.
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
  // Clear the portal FBO to black with zero alpha so anything not covered
  // by scene geometry composites cleanly onto the portal surface (and, for
  // debugging, pixels that should have been filled stand out as obvious
  // holes rather than a deceptive blue-green that used to be a wiring test).
  framebuffer->clear(0.0f, 0.0f, 0.0f, 0.0f);

  // Calculate portal camera view matrix. All portals now carry their own
  // destination_ (possibly == self for mirrors); the unified entry point
  // handles both cases.
  const glm::mat4 portalView =
      PortalCamera::calculatePortalViewUnified(*playerCamera, *sourcePortal);

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

  // Phase 1.5: push mirror-overlay parameters from the Portal into the
  // shader. Uniforms are looked up via glGetUniformLocation rather than
  // Shader::setX so that older portal.fs files (without these uniforms)
  // simply skip the upload instead of logging a warning per frame.
  const GLint mirrorEnabledLoc =
      glGetUniformLocation(currentProgram, "hasMirrorOverlay");
  if (mirrorEnabledLoc >= 0) {
    glUniform1i(mirrorEnabledLoc, portal->hasMirrorOverlay() ? 1 : 0);
  }
  const GLint mirrorIntensityLoc =
      glGetUniformLocation(currentProgram, "mirrorIntensity");
  if (mirrorIntensityLoc >= 0) {
    glUniform1f(mirrorIntensityLoc, portal->getMirrorIntensity());
  }
  const GLint mirrorTintLoc =
      glGetUniformLocation(currentProgram, "mirrorTint");
  if (mirrorTintLoc >= 0) {
    const glm::vec3 tint = portal->getMirrorTint();
    glUniform3f(mirrorTintLoc, tint.x, tint.y, tint.z);
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

// ---------------------------------------------------------------------------
// Stencil-based nested portal rendering (Phase 1.4 option B).
// ---------------------------------------------------------------------------
//
// The high-level shape of the algorithm is documented on
// `PortalRenderer::renderWithStencil` in the header. These helpers are
// intentionally small: each phase (increment, recurse, decrement, clear depth,
// render world) maps to one contiguous GL state change + one draw call so the
// state machine stays readable when debugging in a frame capture.

void PortalRenderer::ensureStencilResources() {
  if (!stencilOnlyShader_ || !stencilOnlyShader_->isValid()) {
    // Minimal vertex shader: transform portal quad vertices by MVP. The
    // fragment shader is empty — color writes are expected to be masked
    // off by the caller, so we don't bother emitting a fragment.
    const std::string vs =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "  gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\n";
    const std::string fs =
        "#version 330 core\n"
        "void main() {}\n";
    stencilOnlyShader_ = Shader::fromString(3, 3, vs, fs);
    if (!stencilOnlyShader_ || !stencilOnlyShader_->isValid()) {
      OMEGA_LOG_ERROR("portal", "Failed to compile stencil-only shader");
      stencilOnlyShader_.reset();
    }
  }

  if (!depthClearShader_ || !depthClearShader_->isValid()) {
    // Fullscreen NDC quad at z=1 (far plane). Used inside the stencil mask
    // to reset depth before rendering the level-L scene over the level-N
    // render we produced during recursion.
    const std::string vs =
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "void main() {\n"
        "  gl_Position = vec4(aPos, 1.0, 1.0);\n"
        "}\n";
    const std::string fs =
        "#version 330 core\n"
        "void main() {}\n";
    depthClearShader_ = Shader::fromString(3, 3, vs, fs);
    if (!depthClearShader_ || !depthClearShader_->isValid()) {
      OMEGA_LOG_ERROR("portal", "Failed to compile depth-clear shader");
      depthClearShader_.reset();
    }
  }

  if (fullscreenQuadVAO_ == 0) {
    // Triangle strip covering NDC [-1,1]^2. Drawn with GL_TRIANGLE_STRIP
    // over four vertices.
    const float quad[] = {
      -1.0f, -1.0f,
       1.0f, -1.0f,
      -1.0f,  1.0f,
       1.0f,  1.0f,
    };
    glGenVertexArrays(1, &fullscreenQuadVAO_);
    glGenBuffers(1, &fullscreenQuadVBO_);
    glBindVertexArray(fullscreenQuadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glBindVertexArray(0);
  }
}

std::shared_ptr<Object> PortalRenderer::ensurePortalQuadObject(
    std::shared_ptr<Portal> portal) {
  if (!portal) return nullptr;
  auto it = portalSurfaces_.find(portal);
  if (it != portalSurfaces_.end()) return it->second;
  // PortalSurface::createSurface emits a position/normal/uv vertex stream;
  // attrib 0 is position which is the only one we read in the stencil-only
  // shader. We pass nullptr for the shader because the Object here is only
  // used as a VAO container — rendering goes through our own shader path.
  auto obj = PortalSurface::createSurface(portal, nullptr);
  if (obj) portalSurfaces_[portal] = obj;
  return obj;
}

void PortalRenderer::drawPortalQuadStencilOnly(
    std::shared_ptr<Portal> portal, std::shared_ptr<Camera> camera) {
  if (!portal || !camera || !stencilOnlyShader_) return;

  auto surfaceObj = ensurePortalQuadObject(portal);
  if (!surfaceObj) return;

  const unsigned int vao = surfaceObj->getVAO();
  const unsigned int count = surfaceObj->getCount();
  const ObjectType type = surfaceObj->getType();
  if (vao == 0 || count == 0) return;

  stencilOnlyShader_->use();
  // Portal quad vertices are baked in world space by PortalSurface, so the
  // model matrix is effectively the object's `model_` (identity by default,
  // but we forward it for correctness in case a future pass starts
  // transforming portal objects).
  glm::mat4 projection = camera->projectionMatrix();
  glm::mat4 view = camera->viewMatrix();
  glm::mat4 model = surfaceObj->getModel();
  stencilOnlyShader_->setMat4fv("projection", projection);
  stencilOnlyShader_->setMat4fv("view", view);
  stencilOnlyShader_->setMat4fv("model", model);

  glBindVertexArray(vao);
  if (type == ObjectType::Elements) {
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, count);
  }
  glBindVertexArray(0);
}

void PortalRenderer::drawDepthClearFullscreenQuad() {
  if (!depthClearShader_ || fullscreenQuadVAO_ == 0) return;
  depthClearShader_->use();
  glBindVertexArray(fullscreenQuadVAO_);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

std::vector<std::shared_ptr<Portal>> PortalRenderer::gatherVisiblePortals(
    std::shared_ptr<Camera> camera,
    std::shared_ptr<Portal> peerPortal) const {
  std::vector<std::shared_ptr<Portal>> out;
  if (!camera) return out;

  auto consider = [&](const std::shared_ptr<Portal> &portal) {
    if (!portal) return;
    // Don't stencil the pair we just came through — that would immediately
    // recurse back into the current level on the next step.
    if (portal == peerPortal) return;
    if (peerPortal && portal == peerPortal->getDestination()) return;
    if (!portal->isOpen() || !portal->isEnabled()) return;
    if (!PortalCamera::isPortalVisible(*portal, *camera)) return;
    if (!PortalCamera::isInViewFrustum(*portal, *camera)) return;
    // activePortals_ is updated by the recursive caller so nested calls
    // skip anything already in-flight higher up the stack.
    if (activePortals_.count(portal) > 0) return;
    out.push_back(portal);
  };

  for (auto &portal : portals_) consider(portal);
  return out;
}

void PortalRenderer::renderWithStencil(std::shared_ptr<Scene> scene,
                                       std::shared_ptr<Camera> camera) {
  if (!enabled_ || !scene || !camera) return;

  activePortals_.clear();
  ensureStencilResources();
  if (!stencilOnlyShader_ || !depthClearShader_ || fullscreenQuadVAO_ == 0) {
    OMEGA_LOG_ERROR(
        "portal",
        "Stencil resources failed to initialise; skipping portal pass");
    return;
  }

  // Clear the whole target (color + depth + stencil) for the recursion
  // frame. We do this here rather than relying on the caller because the
  // stencil algorithm needs a known zero-value starting point for its
  // reference-level counting.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glEnable(GL_STENCIL_TEST);
  // Ensure stencil writes are unmasked by default — individual phases below
  // tighten or slacken this, but we want a clean baseline in case the last
  // frame left some other mask in place.
  glStencilMask(0xFF);

  drawPortalsStencil(scene, camera, 0, nullptr);

  glDisable(GL_STENCIL_TEST);
  glStencilMask(0xFF);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
}

void PortalRenderer::drawPortalsStencil(std::shared_ptr<Scene> scene,
                                        std::shared_ptr<Camera> camera,
                                        int level,
                                        std::shared_ptr<Portal> peerPortal) {
  if (!scene || !camera) return;

  // Collect the portals we can see from this camera at this level.
  // peerPortal excludes itself + its destination to block immediate A↔B
  // loops; activePortals_ blocks deeper cycles.
  const auto visiblePortals = gatherVisiblePortals(camera, peerPortal);

  // --- Phase 1: for each visible portal, stencil-mask it, recurse, unmask.
  for (auto &portal : visiblePortals) {
    activePortals_.insert(portal);

    // 1a. Increment stencil inside the portal quad where it currently
    // equals `level`. Color and depth are frozen.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_EQUAL, level, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glStencilMask(0xFF);
    drawPortalQuadStencilOnly(portal, camera);

    // 1b. Recurse into the destination if we still have depth budget.
    if (level + 1 < maxRecursionDepth_) {
      auto dest = portal->getDestination();
      if (dest) {
        // All portals now carry their own destination_. The unified helper
        // handles mirror (dest == self) and doorway (dest != self) cases.
        const glm::mat4 virtualView =
            PortalCamera::calculatePortalViewUnified(*camera, *portal);
        // PortalViewCamera inherits the base camera's projection matrix
        // (which already has the correct window aspect ratio baked in) —
        // no `setPerspective` needed when rendering into the main
        // framebuffer.
        auto virtualCam =
            std::make_shared<PortalViewCamera>(camera, virtualView);
        drawPortalsStencil(scene, virtualCam, level + 1, dest);
      }
    }

    // 1c. Decrement the stencil so sibling portals (and the parent level)
    // see the counter at `level` again. The DECR is gated on (stencil ==
    // level+1) to only touch pixels we just incremented — sibling portals
    // that overlapped us would have their own increments preserved.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_EQUAL, level + 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    glStencilMask(0xFF);
    drawPortalQuadStencilOnly(portal, camera);

    activePortals_.erase(portal);
  }

  // --- Phase 2: reset depth inside the level-L mask.
  // After all the recursive renders above, the depth buffer inside the
  // portal quads holds fragments from the deepest level we drew. We need
  // the level-L scene render in phase 3 to occlude those, so we force the
  // depth back to 1.0 (far plane) in the level-L mask.
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_EQUAL, level, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilMask(0x00);  // don't touch stencil during the depth clear
  glDepthFunc(GL_ALWAYS);
  drawDepthClearFullscreenQuad();
  glDepthFunc(GL_LESS);

  // --- Phase 3: render the scene at this level, constrained by stencil.
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_EQUAL, level, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilMask(0x00);

  if (peerPortal) {
    // Level > 0: clip to the destination portal's half-space so geometry
    // on the "camera side" of the destination doesn't peek through.
    scene->setActiveClippingPlane(peerPortal->getClippingPlane(), true);
  }
  scene->render(camera);
  if (peerPortal) {
    scene->setActiveClippingPlane(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), false);
  }
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

  // Get destination portal (may be self for mirrors, or null for portals
  // that are enabled but never linked — in which case skip).
  auto dest = portal->getDestination();
  if (!dest) {
    activePortals_.erase(portal);
    return;
  }

  // Render portal view
  renderPortalView(portal, dest, scene, playerCamera, recursionDepth);

  // Recursively render nested portals visible through this portal.
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

