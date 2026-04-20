#pragma once

#include <system/Global.h>
#include <geometry/Portal.h>
#include <geometry/PortalPair.h>
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
   * Add a portal pair to be rendered (legacy: for backward compatibility)
   */
  void addPortalPair(std::shared_ptr<PortalPair> portalPair);

  /**
   * Add a standalone portal to be rendered (new doorway-based system)
   */
  void addPortal(std::shared_ptr<Portal> portal);

  /**
   * Clear all portal pairs and standalone portals
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

  // Legacy: PortalPairs for backward compatibility
  std::vector<std::shared_ptr<PortalPair>> portalPairs_;
  
  // New: Standalone portals (doorway-based system)
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
};

}  // namespace geometry
}  // namespace omega

