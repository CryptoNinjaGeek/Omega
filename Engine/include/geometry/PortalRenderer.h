#pragma once

#include <system/Global.h>
#include <geometry/Portal.h>
#include <geometry/Scene.h>
#include <geometry/Object.h>
#include <render/Camera.h>
#include <render/PortalFramebuffer.h>
#include <render/PortalCamera.h>
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace omega {
namespace render {
class Shader;
}
namespace geometry {

/**
 * PortalRenderer - Handles rendering of portals
 * Manages portal view rendering and portal surface rendering
 */
class OMEGA_EXPORT PortalRenderer {
public:
  PortalRenderer();
  ~PortalRenderer() = default;

  /**
   * Render portal views to framebuffers (call BEFORE main scene render)
   * @param scene The scene to render portals for
   * @param playerCamera The player's camera
   * @param portalShader Shader to use for rendering portal surfaces (unused here, kept for compatibility)
   */
  void renderPortals(std::shared_ptr<Scene> scene,
                     std::shared_ptr<render::Camera> playerCamera,
                     std::shared_ptr<render::Shader> portalShader = nullptr);

  /**
   * Render portal surfaces with framebuffer textures (call AFTER main scene render)
   * @param playerCamera The player's camera
   * @param portalShader Shader to use for rendering portal surfaces
   */
  void renderPortalSurfaces(std::shared_ptr<render::Camera> playerCamera,
                            std::shared_ptr<render::Shader> portalShader = nullptr);

  /**
   * Add a standalone portal to be rendered. Portals loaded through
   * `PortalSceneLoader` are added via this method; destinations are looked
   * up per-portal at render time from `Portal::getDestination()`.
   */
  void addPortal(std::shared_ptr<Portal> portal);

  /**
   * Clear all standalone portals.
   */
  void clearPortals();

  /**
   * Set maximum recursion depth for portal rendering
   */
  void setMaxRecursionDepth(int depth) { maxRecursionDepth_ = depth; }

  /**
   * Enable/disable portal rendering
   */
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

  /**
   * Toggle the stencil-based portal renderer (Phase 1.4 option B). When
   * enabled, Scene::render() routes the portal pass through
   * `renderWithStencil()` instead of the FBO-per-recursion path. Turning
   * this on requires a window framebuffer with a stencil attachment — see
   * `Window::Window()` which requests `GLFW_STENCIL_BITS = 8`.
   */
  void setStencilMode(bool enabled) { stencilMode_ = enabled; }
  bool isStencilMode() const { return stencilMode_; }

  /**
   * Render the scene using stencil-based nested portal rendering. This is
   * the entry point for option B of Phase 1.4 and bypasses the FBO pool
   * entirely: portal "windows" are stenciled into the main framebuffer
   * and the scene is recursively re-drawn through each one with the
   * stencil buffer constraining writes.
   *
   * Algorithm (per recursion level `L`, starting at 0 for the root view):
   *   1. For each visible portal at level L:
   *      a. Increment stencil to L+1 inside the portal quad (color/depth
   *         masked off).
   *      b. Recurse into the destination camera at level L+1, passing the
   *         destination portal as the peer (used for oblique clipping in
   *         step 3 of the recursive call).
   *      c. Decrement stencil back to L (so siblings get a clean mask).
   *   2. Clear depth in the level-L mask using a fullscreen quad at the
   *      far plane — needed because we want the L-level scene to draw in
   *      front of whatever was rendered at deeper levels.
   *   3. Render the world via Scene::render(camera) constrained by
   *      `stencil == L`. If we have a peer portal (i.e. L > 0), enable
   *      oblique clipping against its plane so geometry on the wrong
   *      side of the destination is discarded.
   *
   * Caller (Scene::render()) is responsible for establishing the GL
   * context and viewport. This method clears color/depth/stencil itself
   * because each call starts a fresh stencil-recursion frame.
   */
  void renderWithStencil(std::shared_ptr<Scene> scene,
                         std::shared_ptr<render::Camera> camera);

private:
  /**
   * Render view through a portal
   */
  void renderPortalView(std::shared_ptr<Portal> sourcePortal,
                       std::shared_ptr<Portal> destPortal,
                       std::shared_ptr<Scene> scene,
                       std::shared_ptr<render::Camera> playerCamera,
                       int recursionDepth = 0);

  /**
   * Render portal surface with framebuffer texture
   */
  void renderPortalSurface(std::shared_ptr<Portal> portal,
                           std::shared_ptr<render::Camera> playerCamera,
                           std::shared_ptr<render::Shader> portalShader);

  /**
   * Check if portal is visible from camera
   */
  bool isPortalVisible(std::shared_ptr<Portal> portal,
                      std::shared_ptr<render::Camera> camera) const;

  /**
   * Check if portal should be rendered (includes all culling checks)
   */
  bool shouldRenderPortal(std::shared_ptr<Portal> portal,
                         std::shared_ptr<render::Camera> camera,
                         int recursionDepth) const;

  /**
   * Render portal view recursively (with recursion tracking)
   */
  void renderPortalViewRecursive(std::shared_ptr<Portal> portal,
                                 std::shared_ptr<Scene> scene,
                                 std::shared_ptr<render::Camera> playerCamera,
                                 int recursionDepth);

  // Standalone portals (doorway-based system).
  std::vector<std::shared_ptr<Portal>> portals_;

  int maxRecursionDepth_{3};
  bool enabled_{true};
  
  // Recursion tracking (prevent infinite loops)
  std::set<std::shared_ptr<Portal>> activePortals_;

  // Cache portal surface objects to avoid recreating each frame
  std::map<std::shared_ptr<Portal>, std::shared_ptr<Object>> portalSurfaces_;

  /**
   * Portal surface shader, lazily loaded on first use.
   * Loaded from the canonical paths :/shaders/portal.vs and :/shaders/portal.fs.
   * The previous multi-path fallback chain has been removed; if the shader
   * fails to load the portal surface render is skipped and an error logged.
   */
  std::shared_ptr<render::Shader> portalShader_;

  /**
   * Ensure portalShader_ is loaded. Returns the shader (possibly null on
   * failure). Safe to call repeatedly — subsequent calls are no-ops.
   */
  std::shared_ptr<render::Shader> ensurePortalShader();

  // ----- Stencil-based portal pipeline (Phase 1.4 option B) --------------

  /**
   * Recursive worker for `renderWithStencil`. See the algorithm sketch in
   * the public declaration. `peerPortal` is the destination portal we're
   * currently peering *through* — null at the top level. It's used to
   * drive the oblique clipping plane on the world render and to skip the
   * source+destination pair when gathering nested portals (so an A↔B
   * pair doesn't immediately stencil itself again).
   */
  void drawPortalsStencil(std::shared_ptr<Scene> scene,
                          std::shared_ptr<render::Camera> camera,
                          int level,
                          std::shared_ptr<Portal> peerPortal);

  /**
   * Draw a portal quad into the stencil buffer only. Color and depth
   * writes are expected to be masked off by the caller — this helper just
   * issues the geometry using the minimal `stencilOnlyShader_`.
   */
  void drawPortalQuadStencilOnly(std::shared_ptr<Portal> portal,
                                 std::shared_ptr<render::Camera> camera);

  /**
   * Draw a fullscreen NDC quad at depth = 1.0 (the far plane). Used after
   * stencil-masking to reset depth to the far plane inside the current
   * level's mask, so the level-N scene render can draw in front of the
   * level-(N+1) render we just emitted.
   */
  void drawDepthClearFullscreenQuad();

  /**
   * Lazily compile the two inline shaders (`stencilOnlyShader_`,
   * `depthClearShader_`) and create the fullscreen-quad VAO. Safe to call
   * repeatedly — each resource is only created once.
   */
  void ensureStencilResources();

  /**
   * Ensure a portal quad Object (VAO+VBO+EBO holding the four corner
   * vertices) exists for this portal, reusing the same cache used for
   * the FBO path (`portalSurfaces_`). The stencil-only shader only reads
   * the position attribute, so the richer vertex layout produced by
   * `PortalSurface::createSurface` is compatible.
   */
  std::shared_ptr<Object> ensurePortalQuadObject(std::shared_ptr<Portal> portal);

  /**
   * Collect portals visible from the given camera at the current
   * recursion level. Applies the same culling as `shouldRenderPortal`
   * (open/enabled, facing, in-frustum) plus the peer-exclusion needed by
   * the stencil algorithm: a portal is skipped if it IS the peer portal
   * or its destination (the pair we just came through).
   */
  std::vector<std::shared_ptr<Portal>> gatherVisiblePortals(
      std::shared_ptr<render::Camera> camera,
      std::shared_ptr<Portal> peerPortal) const;

  /**
   * Stencil-mode flag and GL resources. All three resources are created
   * lazily on the first `renderWithStencil` call, so nothing is spun up
   * if the renderer is only ever used in FBO mode.
   */
  bool stencilMode_{false};
  std::shared_ptr<render::Shader> stencilOnlyShader_;
  std::shared_ptr<render::Shader> depthClearShader_;
  unsigned int fullscreenQuadVAO_{0};
  unsigned int fullscreenQuadVBO_{0};
};

}  // namespace geometry
}  // namespace omega

