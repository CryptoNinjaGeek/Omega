#pragma once

#include <system/Global.h>
#include <glad/glad.h>

namespace omega {
namespace render {

/**
 * PortalFramebuffer - Wrapper class for OpenGL Framebuffer Objects (FBOs)
 * Used for off-screen rendering of portal views
 */
class OMEGA_EXPORT PortalFramebuffer {
public:
  PortalFramebuffer(int width = 1024, int height = 1024);
  ~PortalFramebuffer();

  // Disable copy constructor and assignment (manage OpenGL resources)
  PortalFramebuffer(const PortalFramebuffer&) = delete;
  PortalFramebuffer& operator=(const PortalFramebuffer&) = delete;

  // Move constructor and assignment
  PortalFramebuffer(PortalFramebuffer&& other) noexcept;
  PortalFramebuffer& operator=(PortalFramebuffer&& other) noexcept;

  // Bind/unbind framebuffer
  void bind() const;
  void unbind() const;

  // Get framebuffer texture ID (for use in shaders)
  unsigned int getColorTexture() const { return colorTexture_; }
  unsigned int getDepthTexture() const { return depthTexture_; }

  // Get dimensions
  int getWidth() const { return width_; }
  int getHeight() const { return height_; }

  // Resize framebuffer
  void resize(int width, int height);

  // Clear framebuffer
  void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) const;

  // Check if framebuffer is complete
  bool isComplete() const;

  // Get framebuffer ID (for advanced usage)
  unsigned int getFBO() const { return fbo_; }

private:
  void createFramebuffer();
  void destroyFramebuffer();

  unsigned int fbo_{0};
  unsigned int colorTexture_{0};
  unsigned int depthTexture_{0};
  int width_;
  int height_;
  bool valid_{false};
  
  // Mutable to allow modification in const methods (caching OpenGL state)
  mutable GLint savedViewport_[4]{0, 0, 0, 0};
};

}  // namespace render
}  // namespace omega

