#pragma once

// Heightmap — CPU-side authority for terrain elevation data. Samples live in
// a row-major float grid with values nominally in [0,1] (0 = lowest terrain,
// 1 = highest). A HeightmapTransform maps the normalized grid into world
// coordinates so callers can query world-space height/normal directly.
//
// This class is intentionally pure-math: no OpenGL, no GLSL, no scene-graph
// coupling. It is used by the future Terrain system to bake mesh geometry,
// by the TerrainCameraController for analytical ground-follow, and by tests.
//
// Loading goes through FileSystem (zip + disk fallback) via stb_image, matching
// the rest of the engine's image pipeline.

#include <memory>
#include <string>
#include <vector>

#include <system/Global.h>

#include <glm/glm.hpp>

namespace omega {
namespace world {

// World-space mapping for a Heightmap. Normalized coordinates (u, v) ∈ [0,1]
// map to world X/Z, and normalized height h ∈ [0,1] maps to world Y via the
// vertical scale + offset.
struct HeightmapTransform {
  // World-space (x, z) of the UV=(0, 0) corner of the heightmap.
  glm::vec2 origin{0.0f, 0.0f};

  // Total world-space span along X and Z for the whole heightmap. A square
  // heightmap with horizontalScale=1024 covers 1024 world units on each axis.
  float horizontalScale{1.0f};

  // World-space height range corresponding to normalized h ∈ [0, 1].
  float verticalScale{1.0f};

  // World Y added to every sampled height. Usually the sea-level datum.
  float verticalOffset{0.0f};
};

class OMEGA_EXPORT Heightmap {
 public:
  Heightmap() = default;

  // Build from an already-decoded grid of samples in [0, 1]. Width and height
  // must each be >= 2 (the interpolators need at least one cell); otherwise
  // the resulting Heightmap is invalid (valid() returns false).
  Heightmap(std::vector<float> samples, int width, int height,
            HeightmapTransform transform = {});

  // Load a heightmap from a grayscale or color PNG/JPG/BMP via stb_image.
  // Color sources use the red channel (standard convention). Goes through
  // fs::instance()->data() so `:/heightmaps/foo.png` and disk-relative paths
  // both work. Returns nullptr on failure (Log error emitted).
  static std::shared_ptr<Heightmap> load(const std::string& fileName,
                                         HeightmapTransform transform = {});

  // Dimensions of the sample grid.
  int width() const { return width_; }
  int height() const { return height_; }
  bool valid() const { return width_ >= 2 && height_ >= 2 &&
                              static_cast<int>(samples_.size()) ==
                                  width_ * height_; }

  const HeightmapTransform& transform() const { return transform_; }
  void setTransform(const HeightmapTransform& t) { transform_ = t; }

  // Raw grid access. Out-of-range indices are clamped to the nearest edge.
  float sample(int x, int z) const;
  const std::vector<float>& samples() const { return samples_; }

  // Bilinear sample in normalized UV space. u ∈ [0,1] maps to column 0..width-1,
  // v ∈ [0,1] maps to row 0..height-1. Out-of-range UVs are clamped — heightmaps
  // don't wrap by default.
  float sampleBilinear(float u, float v) const;

  // Catmull-Rom bicubic sample. Use for mesh baking where C^1 continuity across
  // chunk boundaries matters. Clamped at edges. Slightly sharper/smoother than
  // bilinear and a little more expensive (4x4 tap versus 2x2).
  float sampleBicubic(float u, float v) const;

  // Surface normal at UV, derived from central-difference gradients. The
  // HeightmapTransform matters here: stretching the horizontal scale without
  // stretching the vertical scale makes slopes gentler. Unit length, Y-up.
  glm::vec3 sampleNormal(float u, float v) const;

  // Slope in [0, 1], where 0 is perfectly flat (normal points up) and values
  // approach 1 as the surface becomes vertical. Equal to 1 - normal.y.
  float sampleSlope(float u, float v) const;

  // World-space convenience wrappers. heightAtWorld applies the full transform
  // (horizontal mapping, vertical scale, vertical offset); normalAtWorld
  // applies the horizontal mapping when deriving the normal.
  float heightAtWorld(float worldX, float worldZ) const;
  glm::vec3 normalAtWorld(float worldX, float worldZ) const;

  // Separable Gaussian smoothing of the heightmap in place. Radius is the
  // half-width of the kernel in samples (>= 1). Larger radii are smoother.
  // Sums to 1 so a constant field stays constant. Intended as a "smoothed
  // out" pre-pass on load; not meant to be called every frame.
  void gaussianBlur(int radius);

 private:
  std::vector<float> samples_;
  int width_{0};
  int height_{0};
  HeightmapTransform transform_{};

  int clampX(int x) const;
  int clampZ(int z) const;
  glm::vec2 worldToUv(float worldX, float worldZ) const;

  // Central-difference gradient in world units: returns (dH/dx, dH/dz) where
  // H is the world-space height function. Used by sampleNormal/normalAtWorld.
  glm::vec2 gradientWorld(float u, float v) const;

  // Catmull-Rom cubic interpolation of four samples at fractional position t.
  static float cubic(float v0, float v1, float v2, float v3, float t);
};

}  // namespace world
}  // namespace omega
