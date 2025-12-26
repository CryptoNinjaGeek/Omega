#include <render/PortalFramebuffer.h>
#include <iostream>

using namespace omega::render;

PortalFramebuffer::PortalFramebuffer(int width, int height)
    : width_(width), height_(height) {
  createFramebuffer();
}

PortalFramebuffer::~PortalFramebuffer() {
  destroyFramebuffer();
}

PortalFramebuffer::PortalFramebuffer(PortalFramebuffer&& other) noexcept
    : fbo_(other.fbo_),
      colorTexture_(other.colorTexture_),
      depthTexture_(other.depthTexture_),
      width_(other.width_),
      height_(other.height_),
      valid_(other.valid_) {
  // Reset other object
  other.fbo_ = 0;
  other.colorTexture_ = 0;
  other.depthTexture_ = 0;
  other.valid_ = false;
}

PortalFramebuffer& PortalFramebuffer::operator=(PortalFramebuffer&& other) noexcept {
  if (this != &other) {
    destroyFramebuffer();

    fbo_ = other.fbo_;
    colorTexture_ = other.colorTexture_;
    depthTexture_ = other.depthTexture_;
    width_ = other.width_;
    height_ = other.height_;
    valid_ = other.valid_;

    other.fbo_ = 0;
    other.colorTexture_ = 0;
    other.depthTexture_ = 0;
    other.valid_ = false;
  }
  return *this;
}

void PortalFramebuffer::createFramebuffer() {
  // Generate framebuffer
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  // Create color texture
  glGenTextures(1, &colorTexture_);
  glBindTexture(GL_TEXTURE_2D, colorTexture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Attach color texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colorTexture_, 0);

  // Create depth texture
  glGenTextures(1, &depthTexture_);
  glBindTexture(GL_TEXTURE_2D, depthTexture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width_, height_, 0,
               GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Attach depth texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthTexture_, 0);

  // Check framebuffer completeness
  valid_ = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  if (!valid_) {
    std::cerr << "ERROR::PORTAL_FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    std::cerr << "Framebuffer status: " << status << std::endl;
  }

  // Unbind framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void PortalFramebuffer::destroyFramebuffer() {
  if (fbo_ != 0) {
    glDeleteFramebuffers(1, &fbo_);
    fbo_ = 0;
  }
  if (colorTexture_ != 0) {
    glDeleteTextures(1, &colorTexture_);
    colorTexture_ = 0;
  }
  if (depthTexture_ != 0) {
    glDeleteTextures(1, &depthTexture_);
    depthTexture_ = 0;
  }
  valid_ = false;
}

void PortalFramebuffer::bind() const {
  // Store current viewport before binding
  glGetIntegerv(GL_VIEWPORT, savedViewport_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, width_, height_);
}

void PortalFramebuffer::unbind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // Restore previous viewport
  glViewport(savedViewport_[0], savedViewport_[1], 
             savedViewport_[2], savedViewport_[3]);
}

void PortalFramebuffer::resize(int width, int height) {
  if (width == width_ && height == height_) {
    return;  // No resize needed
  }

  width_ = width;
  height_ = height;

  // Recreate framebuffer with new size
  destroyFramebuffer();
  createFramebuffer();
}

void PortalFramebuffer::clear(float r, float g, float b, float a) const {
  // Don't call bind() here - assume framebuffer is already bound
  // This prevents overwriting the saved viewport
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool PortalFramebuffer::isComplete() const {
  return valid_;
}

