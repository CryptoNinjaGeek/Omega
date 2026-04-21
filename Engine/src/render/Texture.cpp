#define STB_IMAGE_IMPLEMENTATION
#include <render/Texture.h>
#include <system/FileSystem.h>

#include <stb_image.h>
#include <system/Log.h>
#include <glad/glad.h>
using namespace omega::render;

namespace {

// Translate TextureSpec::Wrap to the GL enum. Kept local so the header stays
// free of <glad/glad.h>; this function is only called from the .cpp.
GLint toGlWrap(TextureSpec::Wrap w) {
  switch (w) {
    case TextureSpec::Wrap::ClampToEdge:     return GL_CLAMP_TO_EDGE;
    case TextureSpec::Wrap::MirroredRepeat:  return GL_MIRRORED_REPEAT;
    case TextureSpec::Wrap::Repeat:
    default:                                 return GL_REPEAT;
  }
}

// Magnification only has LINEAR / NEAREST — mipmapping is a minification
// concern only. Keep the two filter translations separate to make that
// distinction readable at the call site.
GLint toGlMagFilter(TextureSpec::Filter f) {
  return (f == TextureSpec::Filter::Nearest) ? GL_NEAREST : GL_LINEAR;
}

GLint toGlMinFilter(TextureSpec::Filter f, bool mipmaps) {
  if (!mipmaps) {
    return (f == TextureSpec::Filter::Nearest) ? GL_NEAREST : GL_LINEAR;
  }
  // Trilinear for LINEAR, nearest-mipmap-nearest for NEAREST. Matches the
  // legacy hardcoded choice when f==Linear and mipmaps==true.
  return (f == TextureSpec::Filter::Nearest) ? GL_NEAREST_MIPMAP_NEAREST
                                             : GL_LINEAR_MIPMAP_LINEAR;
}

// Pick the (internalFormat, srcFormat) pair from spec + decoded channels.
//
// - 1 channel → always GL_R8 (linear). Single-channel sRGB has no GL
//   internal format; gamma-decoding a grayscale mask is not worth the
//   branch. Caller is expected to know AO/roughness are grayscale masks.
// - 3 channel → GL_SRGB8 for sRGB spec, GL_RGB8 for linear.
// - 4 channel → GL_SRGB8_ALPHA8 for sRGB, GL_RGBA8 for linear.
// Any other channel count falls back to GL_RGB8.
struct FormatPair { GLint internalFormat; GLenum srcFormat; };

FormatPair pickFormat(const TextureSpec& spec, int channels) {
  const bool srgb = (spec.colorSpace == TextureSpec::ColorSpace::sRGB);
  switch (channels) {
    case 1: return {GL_R8, GL_RED};
    case 3: return {srgb ? GL_SRGB8        : GL_RGB8,  GL_RGB};
    case 4: return {srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, GL_RGBA};
    default:
      OMEGA_LOG_WARN("texture",
                     "Unexpected channel count {} — defaulting to RGB8",
                     channels);
      return {GL_RGB8, GL_RGB};
  }
}

}  // namespace

Texture::Texture() {}

Texture::~Texture() {
  // m_textureId==0 is the "never loaded" or "moved-from" state; glDeleteTextures
  // on 0 is a no-op per the spec, but skipping the call keeps the log quiet and
  // avoids a stray GL call when no context is current (e.g. unit tests).
  if (m_textureId != 0) {
    glDeleteTextures(1, &m_textureId);
    m_textureId = 0;
  }
}

Texture::Texture(Texture&& other) noexcept
    : m_textureId(other.m_textureId), _name(std::move(other._name)) {
  // Null the source's handle so its destructor can't free the GL id we
  // just stole. Move-from instances are still destructible, just empty.
  other.m_textureId = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
  if (this != &other) {
    if (m_textureId != 0) {
      glDeleteTextures(1, &m_textureId);
    }
    m_textureId = other.m_textureId;
    _name = std::move(other._name);
    other.m_textureId = 0;
  }
  return *this;
}

auto Texture::loadImageData(const std::string& fileName, bool flip,
                            const std::string& name) -> ImageInfo {
  return loadImageData(fileName, flip, /*forceChannels=*/0, name);
}

auto Texture::loadImageData(const std::string& fileName, bool flip,
                            int forceChannels, const std::string& name)
    -> ImageInfo {
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(flip);
  auto bytes = fs::instance()->data(fileName);

  unsigned char *data =
      stbi_load_from_memory(bytes.data(), bytes.size(), &width, &height,
                            &nrChannels, forceChannels);

  // When forceChannels is non-zero stb reports the *source* channel count in
  // `nrChannels` but returns pixel data with exactly forceChannels channels.
  // The downstream upload path needs the effective channel count, so report
  // that instead.
  const int effectiveChannels =
      (forceChannels > 0) ? forceChannels : nrChannels;

  return {
      .data = data, .width = width, .height = height,
      .channels = effectiveChannels};
}

bool Texture::load(const std::string& fileName, const std::string& name) {
  // Legacy path — same behaviour as before TextureSpec existed. Any call
  // site that wants explicit sRGB / linear / wrap / filter control should
  // switch to the spec overload; this signature stays as the "diffuse art"
  // convenience and is preserved byte-compatibly.
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_2D, m_textureId);
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_set_flip_vertically_on_load(true);

  auto imageInfo = loadImageData(fileName);
  if (imageInfo.data) {
    int format{GL_RGB};
    if (imageInfo.channels==1)
      format = GL_RED;
    else if (imageInfo.channels==3)
      format = GL_RGB;
    else if (imageInfo.channels==4)
      format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, imageInfo.width, imageInfo.height, 0,
                 format, GL_UNSIGNED_BYTE, imageInfo.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    _name = name.empty() ? fileName : name;
  } else {
    OMEGA_LOG_ERROR("texture", "Failed to load texture: {}", fileName);
  }
  stbi_image_free(imageInfo.data);
  return true;
}

bool Texture::load(const std::string& fileName, const TextureSpec& spec,
                   const std::string& name) {
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_2D, m_textureId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGlWrap(spec.wrapS));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGlWrap(spec.wrapT));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  toGlMinFilter(spec.minFilter, spec.generateMipmaps));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  toGlMagFilter(spec.magFilter));

  auto imageInfo =
      loadImageData(fileName, spec.flipY, spec.forceChannels, name);
  if (!imageInfo.data) {
    OMEGA_LOG_ERROR("texture", "Failed to load texture: {}", fileName);
    // Don't leak the GL id we allocated — either the ctor-produced fallback
    // path runs (callers supply createWhiteTexture) or the object will be
    // discarded and ~Texture frees it. Either way, zero-ing here means an
    // activate() on a failed load binds GL_TEXTURE_2D 0 instead of an
    // uninitialised id.
    glDeleteTextures(1, &m_textureId);
    m_textureId = 0;
    return false;
  }

  const auto fmt = pickFormat(spec, imageInfo.channels);
  glTexImage2D(GL_TEXTURE_2D, 0, fmt.internalFormat,
               imageInfo.width, imageInfo.height, 0, fmt.srcFormat,
               GL_UNSIGNED_BYTE, imageInfo.data);

  if (spec.generateMipmaps) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  _name = name.empty() ? fileName : name;
  stbi_image_free(imageInfo.data);
  return true;
}

bool Texture::activate(int no) {
  // bind textures on corresponding texture units
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(GL_TEXTURE_2D, m_textureId);
  return true;
}

std::shared_ptr<Texture> Texture::createWhiteTexture() {
  auto texture = std::make_shared<Texture>();

  // Create a 1x1 white texture using OpenGL
  glGenTextures(1, &texture->m_textureId);
  glBindTexture(GL_TEXTURE_2D, texture->m_textureId);

  unsigned char whiteData[4] = {255, 255, 255, 255}; // RGBA white
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whiteData);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  texture->_name = "default_white";

  return texture;
}

namespace {

// Shared helper for the default_* factories. Uploads a 1x1 RGBA value into
// a fresh GL texture with GL_REPEAT / GL_LINEAR / no mipmaps. Any neutral
// PBR fallback fits this shape — it's the per-channel value that differs.
std::shared_ptr<Texture> makeSolid1x1(const char* debugName,
                                      unsigned char r,
                                      unsigned char g,
                                      unsigned char b,
                                      unsigned char a) {
  auto tex = std::make_shared<Texture>();
  unsigned int id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  const unsigned char pixel[4] = {r, g, b, a};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixel);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Inject the GL id into the Texture instance. Done via a tiny friend
  // trick: since Texture's fields are protected and we're in the .cpp of
  // the same class's implementation, we can reach them through a local
  // subclass. Cheaper than adding a public setter that nothing else needs.
  struct Back : public Texture {
    using Texture::m_textureId;
    using Texture::_name;
  };
  auto& back = static_cast<Back&>(*tex);
  back.m_textureId = id;
  back._name = debugName;
  return tex;
}

}  // namespace

std::shared_ptr<Texture> Texture::defaultWhite() {
  // Function-local static: lazily allocated on first call, freed at program
  // exit. Crucially, the cache lives for the full process so every Material
  // that wants a white fallback shares the same GL id — no per-material
  // 1x1 leak.
  static std::shared_ptr<Texture> tex =
      makeSolid1x1("default_white_cached", 255, 255, 255, 255);
  return tex;
}

std::shared_ptr<Texture> Texture::defaultFlatNormal() {
  // (0.5, 0.5, 1.0) encoded as bytes. The shader's (x*2 - 1) decode yields
  // tangent-space (0, 0, 1), which after the TBN rotation equals the
  // geometric normal — i.e. no perturbation. The 128 for the .5 channels
  // is the conventional encoding even though 127 is technically closer;
  // the 0.5-byte difference is inside texture LERP noise at low-res.
  static std::shared_ptr<Texture> tex =
      makeSolid1x1("default_flat_normal", 128, 128, 255, 255);
  return tex;
}

std::shared_ptr<Texture> Texture::defaultBlack() {
  static std::shared_ptr<Texture> tex =
      makeSolid1x1("default_black", 0, 0, 0, 255);
  return tex;
}
