#include <render/Material.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <system/FileSystem.h>
#include <system/Log.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

using namespace omega::render;

namespace {

// Resolve `<directory>/<stem><sep><suffix>.<ext>` against the filesystem,
// probing every (separator, extension) combination until one hits. Covers
// the two naming conventions present in the asset packs today:
//
//   Poliigon   — `<stem>_<Suffix>.<jpg|png>`    e.g. grass, dirt
//   Hyphenated — `<stem>-<Suffix>.<jpg|png>`    e.g. snow-packed12-BaseColor.png
//
// Probe order: underscore before hyphen (most packs are Poliigon-style),
// jpg before png (colour-ish maps tend to ship as jpg). A pack that only
// ships one variant still resolves in one or two probes; a pack that mixes
// extensions per-suffix (grass: Normal is png, rest are jpg) resolves
// correctly because each suffix is probed independently.
//
// The lookup is via `omega::fs::instance()->data(candidate)`, which
// consults the disk overlay and then the zip index (see
// `project_resources_zip.md`). A missing file returns a zero-size buffer
// — the cheapest "exists?" probe that doesn't double-decode the image.
std::string resolveFirstExisting(
    const std::string& directory, const std::string& stem,
    const std::string& suffix,
    const std::vector<std::string>& extensions) {
  static const char* const kSeparators[] = {"_", "-"};
  for (const auto* sep : kSeparators) {
    for (const auto& ext : extensions) {
      std::string candidate =
          directory + "/" + stem + sep + suffix + "." + ext;
      auto bytes = omega::fs::instance()->data(candidate);
      if (bytes.size() > 0) return candidate;
    }
  }
  return {};
}

// Load one slot of the PBR set with the right spec. Returns nullptr silently
// if the file isn't on disk (that's the expected case for any slot the
// source asset pack doesn't ship — dielectric materials typically omit
// Metallic, many outdoor sets omit AO, etc.). A *decode* failure on a
// file that did resolve still logs an error, because that's a real
// problem the caller should know about.
std::shared_ptr<Texture> loadOne(const std::string& directory,
                                 const std::string& stem,
                                 const std::string& suffix,
                                 const std::vector<std::string>& extensions,
                                 const TextureSpec& spec,
                                 const std::string& logicalName) {
  const auto path = resolveFirstExisting(directory, stem, suffix, extensions);
  if (path.empty()) return nullptr;  // Expected when the slot isn't provided.
  auto tex = std::make_shared<Texture>();
  if (!tex->load(path, spec, logicalName)) {
    OMEGA_LOG_ERROR("material", "Failed to load PBR texture: {}", path);
    return nullptr;
  }
  return tex;
}

}  // namespace

namespace omega {
namespace render {

int Material::apply(Shader& shader, const std::string& prefix,
                    int firstUnit) const {
  // Uniform naming: an empty prefix maps onto the flat "material.*" names
  // that core.fs historically used, which lets new-API call sites target
  // the old shader without renaming every uniform. A non-empty prefix is
  // used verbatim — callers typically pass "materials[0]" etc. to address
  // one slot in an array-of-struct.
  const std::string root = prefix.empty() ? "material" : prefix;

  // Small struct so the slot loop stays compact. We fall back to a neutral
  // 1x1 texture when a slot is null so the shader can sample blindly; we
  // still emit a `has*` bool so shaders that *do* branch know which slots
  // hold real data.
  struct Slot {
    const char* uniformName;
    const char* hasFlagName;
    const std::shared_ptr<Texture>& bound;
    std::shared_ptr<Texture> fallback;
  };

  const Slot slots[kApplyUnitCount] = {
      {"baseColor",    "hasBaseColor",    baseColor,    Texture::defaultWhite()},
      {"normalMap",    "hasNormalMap",    normalMap,    Texture::defaultFlatNormal()},
      {"roughnessMap", "hasRoughnessMap", roughnessMap, Texture::defaultWhite()},
      {"aoMap",        "hasAoMap",        aoMap,        Texture::defaultWhite()},
      {"metallicMap",  "hasMetallicMap",  metallicMap,  Texture::defaultBlack()},
  };

  for (int i = 0; i < kApplyUnitCount; ++i) {
    const auto& s = slots[i];
    const int unit = firstUnit + i;

    // Pick the real texture if bound, otherwise the neutral fallback. The
    // fallback shared_ptrs are cached inside Texture's defaultXxx() helpers
    // so we reuse the same GL id across every Material instance.
    const auto& tex = s.bound ? s.bound : s.fallback;
    tex->activate(unit);

    shader.setInt(root + "." + s.uniformName, unit);
    // Shader has no setBool — use an int(0/1). GLSL converts 0/non-zero
    // cleanly when the uniform is declared as bool on the shader side.
    shader.setInt(root + "." + s.hasFlagName, s.bound ? 1 : 0);
  }

  // Numeric per-material uniforms. Kept alongside the texture bindings so
  // a single apply() call fully sets up one material slot — the shader
  // never needs to read anything outside `<root>.*` to render this entry.
  shader.setVec4(root + ".color", color);
  shader.setVec3(root + ".diffuse", diffuse);
  shader.setFloat(root + ".opacity", opacity);
  shader.setFloat(root + ".shininess", shininess);
  shader.setVec2(root + ".uvTiling", uvTiling);

  return kApplyUnitCount;
}

// Load a single-texture, non-PBR Material. The texture at `opts.path` goes
// into the `baseColor` slot (same slot the PBR path uses for albedo) so the
// same Material::apply binding contract works for both flavours — a shader
// that only cares about base colour doesn't have to know whether the
// Material was built by loadMaterial or loadPBRMaterial.
//
// Load failure is not fatal: we fall back to the cached white 1x1 so the
// shader side still has something to sample. Texture::load already logs
// the specific load error on its own, so we don't re-log here.
Material loadMaterial(const MaterialLoadOptions& opts) {
  Material out;
  out.shininess = opts.shininess;

  auto tex = std::make_shared<Texture>();
  if (tex->load(opts.path)) {
    out.baseColor = tex;
  } else {
    // Cached fallback — shared across all materials that miss their load.
    out.baseColor = Texture::defaultWhite();
  }
  return out;
}


Material loadPBRMaterial(const PbrLoadOptions& opts) {
  Material out;
  // Apply caller-supplied per-material tweaks up front. Every other
  // Material field (color, diffuse, opacity, uvTiling, …) keeps its
  // struct default; callers mutate the returned Material in-place if
  // they need to change more.
  out.shininess = opts.shininess;

  // Pull the path / stem out of the params struct for readability below.
  const std::string& directory = opts.path;
  const std::string& stem      = opts.name;

  // Extension order: jpg first (Poliigon's common encoding), then png
  // (required for Normal maps in the grass set). TIFF is intentionally not
  // listed — stb_image doesn't support it, and 16-bit height/displacement
  // data needs a separate path we'll add when Phase 4 lands parallax.
  const std::vector<std::string> exts = {"jpg", "png"};

  // Every slot uses Repeat wrap. Terrain tiles materials across the mesh,
  // and that's every current use-case for loadPBRMaterial. A decal-style
  // caller that needs ClampToEdge can post-load the Material or add a
  // second overload when the need actually materialises — we're not
  // pre-exposing knobs for hypothetical callers.
  auto makeSpec = [](TextureSpec::ColorSpace cs, int forceChannels) {
    TextureSpec s;
    s.colorSpace = cs;
    s.wrapS = TextureSpec::Wrap::Repeat;
    s.wrapT = TextureSpec::Wrap::Repeat;
    s.forceChannels = forceChannels;
    return s;
  };

  // Try every standard slot. resolveFirstExisting probes the filesystem
  // per-extension and loadOne returns nullptr silently for slots that
  // aren't present, so the shape of the returned Material simply mirrors
  // what the asset pack ships — no opts, no flags, no policy.

  // BaseColor: authored in sRGB — GL gamma-decodes on sample.
  out.baseColor = loadOne(directory, stem, "BaseColor", exts,
                          makeSpec(TextureSpec::ColorSpace::sRGB, 0),
                          stem + "_BaseColor");

  // Normal: linear tangent-space data. The Poliigon .mtlx tags it
  // srgb_texture but that's a known Poliigon convention bug; the pixel
  // values are raw normal components and must be uploaded linear or the
  // whole tangent frame skews.
  out.normalMap = loadOne(directory, stem, "Normal", exts,
                          makeSpec(TextureSpec::ColorSpace::Linear, 0),
                          stem + "_Normal");

  // Roughness: grayscale linear mask — force single-channel so we upload
  // as GL_R8 rather than wasting an RGB texture on a scalar. The shader
  // samples `.r` accordingly.
  out.roughnessMap = loadOne(directory, stem, "Roughness", exts,
                             makeSpec(TextureSpec::ColorSpace::Linear, 1),
                             stem + "_Roughness");

  // AmbientOcclusion: grayscale mask, force single-channel. The .mtlx
  // marks AO as sRGB; we pick linear here because the spec's sRGB case
  // has no single-channel internal format (see TextureSpec docs). The
  // visual error from treating a multiplier in linear vs sRGB is small
  // enough to be invisible at this engine scope.
  out.aoMap = loadOne(directory, stem, "AmbientOcclusion", exts,
                      makeSpec(TextureSpec::ColorSpace::Linear, 1),
                      stem + "_AmbientOcclusion");

  // Metallic: grayscale linear mask. Dielectric asset packs (grass, wood,
  // plaster) typically ship no Metallic map at all; those slots resolve
  // to nullptr and Material::apply will bind the cached black fallback —
  // which is exactly the right "metallic = 0" default for dielectrics.
  out.metallicMap = loadOne(directory, stem, "Metallic", exts,
                            makeSpec(TextureSpec::ColorSpace::Linear, 1),
                            stem + "_Metallic");

  return out;
}

}  // namespace render
}  // namespace omega
