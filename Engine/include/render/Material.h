#pragma once

#include <memory>
#include <string>
#include <system/Global.h>
#include <glm/glm.hpp>

namespace omega {
namespace render {
class Shader;
class Texture;

struct OMEGA_EXPORT PbrLoadOptions {
  std::string path;
  std::string name;
  float shininess;
};

struct OMEGA_EXPORT MaterialLoadOptions {
  std::string path;
  float shininess;
};

// Material — per-object shading parameters plus optional textures.
//
// Historical state: a single `specular` texture slot alongside numeric
// shininess/diffuse/color/opacity. The legacy Blinn-Phong shader (core.fs)
// and every existing call site (Box, Plane, Container, PortalSceneLoader)
// read only those fields.
//
// Evolution: additive PBR slots for base-colour / normal / roughness / AO
// (and, reserved for later, metallic / height) so shaders that want a PBR
// pipeline can opt in without breaking the legacy path. Legacy fields stay
// put; nothing in core.fs or core.vs has to change.
//
// Slots that are nullptr at render time are treated by the shader as "not
// bound" — typically via a `hasXxxMap` uniform or by sampling a white
// fallback. Each consuming shader decides its own convention.
struct Material {
  // --- Legacy (Blinn-Phong) ------------------------------------------------
  float shininess{16.f};
  std::shared_ptr<Texture> specular;

  // Color properties (from JSON)
  glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};  // RGBA color
  glm::vec3 diffuse{1.0f, 1.0f, 1.0f};     // RGB diffuse color
  float opacity{1.0f};                      // Opacity (0.0-1.0)

  // --- PBR texture slots (optional) ---------------------------------------
  // Every slot is independent — a Material can legitimately have only a
  // baseColor, baseColor + normal, or the full set. Null slots are bound
  // by Material::apply with a neutral 1x1 fallback (white for baseColor /
  // roughness / AO, flat-normal for normal, black for metallic) so the
  // shader side can either (a) sample blindly and get a no-op value, or
  // (b) branch on the `hasXxx` bool uniforms apply() also emits.
  //
  // Colorspace is the loader's responsibility: BaseColor and AO are
  // typically sRGB-encoded source art; Normal/Roughness/Metallic are
  // linear data. See `loadPBRMaterial` below for a helper that gets the
  // conventions right for Poliigon-named asset bundles.
  std::shared_ptr<Texture> baseColor;
  std::shared_ptr<Texture> normalMap;
  std::shared_ptr<Texture> roughnessMap;
  std::shared_ptr<Texture> metallicMap;
  std::shared_ptr<Texture> aoMap;
  std::shared_ptr<Texture> heightMap;  // Reserved for Phase 4+ displacement.

  // Per-material UV tiling (multiplied into the fragment's vertex UV before
  // sampling any of the slots). Lets a shared terrain mesh have one tile
  // rate for the grass band and another for the rock band without needing
  // separate meshes.
  glm::vec2 uvTiling{1.0f, 1.0f};

  // Bind this material to `shader` under the uniform prefix `prefix`,
  // allocating texture units starting at `firstUnit`. Returns the number
  // of texture units consumed (always kApplyUnitCount for now; kept as a
  // return value so callers can compose multiple materials back-to-back
  // without hardcoding the stride).
  //
  // Uniforms set:
  //   <prefix>.baseColor     sampler2D, unit = firstUnit + 0
  //   <prefix>.normalMap     sampler2D, unit = firstUnit + 1
  //   <prefix>.roughnessMap  sampler2D, unit = firstUnit + 2
  //   <prefix>.aoMap         sampler2D, unit = firstUnit + 3
  //   <prefix>.metallicMap   sampler2D, unit = firstUnit + 4
  //   <prefix>.hasBaseColor     int (0/1) — true if a real texture is bound
  //   <prefix>.hasNormalMap     int (0/1)
  //   <prefix>.hasRoughnessMap  int (0/1)
  //   <prefix>.hasAoMap         int (0/1)
  //   <prefix>.hasMetallicMap   int (0/1)
  //   <prefix>.color            vec4
  //   <prefix>.diffuse          vec3
  //   <prefix>.opacity          float
  //   <prefix>.shininess        float
  //   <prefix>.uvTiling         vec2
  //
  // Empty (default) prefix emits the flat "material.*" names historically
  // used by core.fs — useful if a call site wants the new API to populate
  // the old uniforms.
  static constexpr int kApplyUnitCount = 5;
  int apply(Shader& shader, const std::string& prefix, int firstUnit) const;
};

// Load a Poliigon-style PBR set into a Material, given a base directory
// and a common filename stem. The Poliigon naming convention is:
//   <stem>_BaseColor.<ext>
//   <stem>_Normal.<ext>
//   <stem>_Roughness.<ext>
//   <stem>_Metallic.<ext>
//   <stem>_AmbientOcclusion.<ext>
// (extensions vary per map — BaseColor/AO/Roughness/Metallic are jpg,
// Normal is png in the grass set). Both {jpg,png} are probed, in that
// order, so mixed-extension sets work out of the box.
//
// Auto-detection: each of the five standard slots is attempted, and only
// the ones whose files actually exist on the filesystem are loaded.
// Missing slots stay null on the returned Material; Material::apply then
// substitutes its cached neutral 1×1 fallback at bind time so the shader
// side can sample blindly and still get a sensible value. No options are
// exposed — if a Metallic file is present the Material gets one; if it
// isn't, it doesn't. That's the entire contract.
//
// Each slot is uploaded with the colourspace / channel count / wrap that
// PBR convention requires for that role (sRGB 3-channel base colour,
// linear tangent-space normal, R8 roughness/AO/metallic, Repeat wrap).
// Non-texture Material fields (shininess, color, diffuse, opacity,
// uvTiling) keep their struct defaults — callers tweak them in-place on
// the returned Material.
//
// The directory may be a `:/`-prefixed resource path.
OMEGA_EXPORT
Material loadPBRMaterial(const PbrLoadOptions& opts);

OMEGA_EXPORT
Material loadMaterial(const MaterialLoadOptions& opts);

};  // namespace render
};  // namespace omega


