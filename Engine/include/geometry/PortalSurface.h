#pragma once

#include <system/Global.h>
#include <geometry/Portal.h>
#include <geometry/Object.h>
#include <geometry/Vertex.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <memory>
#include <vector>

namespace omega {
namespace geometry {

/**
 * PortalSurface - Renders a portal surface as a quad mesh
 * Creates an Object that can be rendered with the portal framebuffer texture
 */
class OMEGA_EXPORT PortalSurface {
public:
  /**
   * Create a portal surface object from a portal
   * @param portal The portal to create a surface for
   * @param shader Shader to use for rendering the portal surface
   * @return Object that can be added to scene for rendering
   */
  static std::shared_ptr<Object> createSurface(
      std::shared_ptr<Portal> portal,
      std::shared_ptr<render::Shader> shader);

private:
  /**
   * Generate vertices for portal quad
   */
  static void generatePortalVertices(
      const Portal& portal,
      std::vector<geometry::Vertex>& vertices,
      std::vector<unsigned int>& indices);
};

}  // namespace geometry
}  // namespace omega

