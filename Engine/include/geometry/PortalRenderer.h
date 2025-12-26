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
   * Add a portal pair to be rendered
   */
  void addPortalPair(std::shared_ptr<PortalPair> portalPair);

  /**
   * Clear all portal pairs
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

  std::vector<std::shared_ptr<PortalPair>> portalPairs_;
  int maxRecursionDepth_{2};
  bool enabled_{true};
  
  // Cache portal surface objects to avoid recreating each frame
  std::map<std::shared_ptr<Portal>, std::shared_ptr<Object>> portalSurfaces_;
};

}  // namespace geometry
}  // namespace omega

