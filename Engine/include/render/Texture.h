#pragma once

#include <memory>
#include <string>

#include <system/Global.h>

namespace omega {
namespace render {
struct ImageInfo {
  unsigned char *data;
  int width;
  int height;
  int channels;
};

// TextureSpec — per-load configuration for the Texture class.
//
// The legacy `Texture::load(filename, name)` path hardcodes sRGB-ish RGB,
// GL_REPEAT wrap, LINEAR_MIPMAP_LINEAR min / LINEAR mag, and always flips the
// image vertically. That's fine for diffuse/albedo art, but breaks every other
// texture use case: normal maps want linear internal format, roughness /
// metallic / AO are single-channel linear data, etc.
//
// TextureSpec is an additive overload: call sites that want the old behaviour
// keep using `load(file, name)`; anything needing explicit control passes a
// spec. Defaults match the legacy path so an empty spec is equivalent to the
// old load (plus per-texture colorspace picking).
struct TextureSpec {
  enum class ColorSpace {
    // Source pixels are in sRGB. GL decodes to linear when sampling (via
    // GL_SRGB8 / GL_SRGB8_ALPHA8 internal format). Use for BaseColor, AO,
    // anything authored "as seen" in an image editor.
    sRGB,
    // Source pixels are already linear data (normal maps, roughness,
    // metallic, height). No gamma decode — sampler returns the raw values.
    Linear,
  };
  enum class Wrap {
    Repeat,
    ClampToEdge,
    MirroredRepeat,
  };
  enum class Filter {
    Linear,
    Nearest,
  };

  ColorSpace colorSpace{ColorSpace::sRGB};
  Wrap wrapS{Wrap::Repeat};
  Wrap wrapT{Wrap::Repeat};
  Filter minFilter{Filter::Linear};
  Filter magFilter{Filter::Linear};
  // If true, the min filter uses the mipmapped variant (e.g. LINEAR →
  // LINEAR_MIPMAP_LINEAR) and glGenerateMipmap is called after upload.
  bool generateMipmaps{true};
  // Match the legacy path's default: flip the image vertically on load so
  // stb_image's top-left origin matches OpenGL's bottom-left UV (0,0).
  bool flipY{true};
  // 0 = auto-detect from file (legacy behaviour). Otherwise force stb_image
  // to decode to exactly N channels (1/3/4). Useful for roughness / AO where
  // the source jpg is grayscale and we want a GL_R8 single-channel upload.
  int forceChannels{0};

  // Convenience factories for common texture roles. These are just tagged
  // specs — callers can always mutate fields after.
  static TextureSpec srgbColor() { return {}; }
  static TextureSpec linearData() {
    TextureSpec s;
    s.colorSpace = ColorSpace::Linear;
    return s;
  }
  static TextureSpec normalMap() {
    TextureSpec s;
    s.colorSpace = ColorSpace::Linear;
    return s;
  }
  static TextureSpec grayscaleLinear() {
    TextureSpec s;
    s.colorSpace = ColorSpace::Linear;
    s.forceChannels = 1;
    return s;
  }
};

class OMEGA_EXPORT Texture {
public:
  Texture();
  virtual ~Texture();

  // The Texture owns a single GL handle. Copying would double-free in the
  // destructor, and the engine already passes Texture around exclusively
  // via std::shared_ptr<Texture> — no caller holds one by value. Deleting
  // copy ops makes that contract explicit and makes future accidents a
  // compile error rather than a runtime double-delete.
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;

  virtual auto activate(int no) -> bool;

  // Legacy load path: single-file, sRGB-ish, hardcoded sampler config.
  // Preserved verbatim so existing call sites (Box, Plane, Container,
  // PortalSceneLoader, the Castle demo's texture list, etc.) keep compiling
  // and behaving as before.
  auto load(const std::string& fileName, const std::string& name = {}) -> bool;

  // Spec-driven load. Picks internal format (sRGB vs linear, 1/3/4 channel),
  // wrap, filter, mipmap generation, and vertical flip from the spec. Single-
  // channel uploads always use GL_R8 regardless of colorSpace (GL has no
  // sRGB single-channel internal format; gamma on a grayscale multiplier
  // mask is visually a rounding error at this engine scope).
  auto load(const std::string& fileName, const TextureSpec& spec,
            const std::string& name = {}) -> bool;

  // Create a white 1x1 texture programmatically
  static std::shared_ptr<Texture> createWhiteTexture();

  // Cached 1x1 neutral-value textures. Used by Material::apply and other
  // binding paths as "this PBR slot isn't populated, but the shader still
  // expects a bound sampler". The first call lazily allocates; subsequent
  // calls return the same shared_ptr — so a scene with 100 materials that
  // all lack a normal map still only uses one flat-normal GL texture.
  //
  // Fallback values:
  //   defaultWhite()      — RGBA (1,1,1,1). Good for missing baseColor /
  //                          roughness / AO (full brightness / max roughness /
  //                          no occlusion).
  //   defaultFlatNormal() — RGBA (0.5, 0.5, 1, 1) i.e. tangent-space +Z.
  //                          Remapping (x*2-1) in the shader gives (0,0,1),
  //                          so the perturbed normal equals the geometric
  //                          normal (no perturbation).
  //   defaultBlack()      — RGBA (0,0,0,1). Good for missing metallic maps
  //                          (0 = dielectric) and any "absent by default"
  //                          slot the caller wants to treat as zero.
  static std::shared_ptr<Texture> defaultWhite();
  static std::shared_ptr<Texture> defaultFlatNormal();
  static std::shared_ptr<Texture> defaultBlack();

  auto name() -> std::string { return _name; }
  auto name(const std::string& name) -> void { _name = name; }

  // Expose the GL handle for callers that need to bind manually (e.g. FBO
  // attachments). Returns 0 if the texture has not been successfully loaded.
  auto glHandle() const -> unsigned int { return m_textureId; }

protected:
  auto loadImageData(const std::string& fileName, bool flip = true, const std::string& name = {}) -> ImageInfo;
  auto loadImageData(const std::string& fileName, bool flip,
                     int forceChannels, const std::string& name) -> ImageInfo;

protected:
  unsigned int m_textureId{0};
  std::string _name;
};
};  // namespace render
};  // namespace omega
