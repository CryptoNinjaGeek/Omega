#include <world/Heightmap.h>

#include <algorithm>
#include <array>
#include <cmath>

#include <glm/gtc/constants.hpp>

#include <stb_image.h>

#include <system/FileSystem.h>
#include <system/Log.h>

namespace omega {
namespace world {

namespace {

// Minimum sensible kernel radius. A radius of 0 would be a no-op and is more
// likely a caller bug than a valid request; we clamp up rather than return.
constexpr int kMinBlurRadius = 1;

// Clamp UV into [0, 1 - epsilon]. Subtracting an epsilon from the upper bound
// keeps the integer "right neighbor" index valid for bilinear/bicubic taps.
float clampUv(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

// --- Procedural-island helpers ---------------------------------------------
// These back Heightmap::makeProceduralIsland. They're file-local because no
// other Heightmap method needs them and they encode a specific terrain look
// (sum-of-sines + radial falloff) rather than a general primitive.

// Island falloff: samples near the border are pulled down so the result
// reads as a finite island rather than a plane that just ends. Keeping it
// analytic (smoothstep on normalized radius) avoids another texture
// dependency at generation time.
float islandFalloff(float u, float v) {
  const float dx = u - 0.5f;
  const float dz = v - 0.5f;
  const float r = std::sqrt(dx * dx + dz * dz) * 2.0f;  // 0 centre → 1 edge
  // Start pulling down at r=0.75, reach zero by r=1. The inner 75% of the
  // radius is unaffected, so the centre of the map stays hilly.
  return 1.0f - glm::smoothstep(0.75f, 1.0f, r);
}

// Sum-of-sines proc-gen. Cheap, deterministic, produces something that
// reads as natural rolling terrain without shipping an asset. Values are
// clamped to [0, 1] so the downstream HeightmapTransform can map them to
// world space with a single multiply.
float proceduralHeightAt(float u, float v) {
  const float twoPi = glm::two_pi<float>();

  const float low =
      0.5f + 0.5f * std::sin((u * 1.7f + v * 2.1f) * twoPi);
  const float mid =
      0.5f + 0.5f * std::sin((u * 3.3f - v * 2.6f) * twoPi + 1.2f);
  const float high =
      0.5f + 0.5f * std::sin((u * 6.1f + v * 5.4f) * twoPi + 2.8f);
  const float detail =
      0.5f + 0.5f * std::sin((u * 13.0f + v * 11.0f) * twoPi + 0.4f);

  // Weighted sum. The detail octave barely contributes but stops the
  // surface from looking too smooth at grazing angles.
  float h = 0.55f * low + 0.28f * mid + 0.12f * high + 0.05f * detail;
  h *= islandFalloff(u, v);

  if (h < 0.0f) h = 0.0f;
  if (h > 1.0f) h = 1.0f;
  return h;
}

}  // namespace

Heightmap::Heightmap(std::vector<float> samples, int width, int height,
                     HeightmapTransform transform)
    : samples_(std::move(samples)),
      width_(width),
      height_(height),
      transform_(transform) {
  if (static_cast<int>(samples_.size()) != width_ * height_) {
    OMEGA_LOG_ERROR("heightmap",
                    "sample buffer size ({}) does not match {}x{} grid",
                    samples_.size(), width_, height_);
    // Mark invalid by resetting dimensions; valid() will now return false.
    samples_.clear();
    width_ = 0;
    height_ = 0;
  }
}

std::shared_ptr<Heightmap> Heightmap::load(const std::string& fileName,
                                           HeightmapTransform transform) {
  auto bytes = fs::instance()->data(fileName);
  if (bytes.size() == 0 || bytes.data() == nullptr) {
    OMEGA_LOG_ERROR("heightmap", "failed to read {}", fileName);
    return nullptr;
  }

  int width = 0;
  int height = 0;
  int channels = 0;
  // Request a single channel: we only need luminance for a heightmap.
  stbi_set_flip_vertically_on_load(false);
  unsigned char* pixels =
      stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                            &width, &height, &channels, /*req_comp=*/1);
  if (pixels == nullptr || width < 2 || height < 2) {
    OMEGA_LOG_ERROR("heightmap",
                    "stb_image failed or image too small for {} (got {}x{})",
                    fileName, width, height);
    if (pixels != nullptr) stbi_image_free(pixels);
    return nullptr;
  }

  std::vector<float> samples(static_cast<size_t>(width) * height);
  const float inv255 = 1.0f / 255.0f;
  for (size_t i = 0; i < samples.size(); ++i) {
    samples[i] = static_cast<float>(pixels[i]) * inv255;
  }
  stbi_image_free(pixels);

  return std::make_shared<Heightmap>(std::move(samples), width, height,
                                     transform);
}

std::shared_ptr<Heightmap> Heightmap::makeProceduralIsland(
    const ProceduralIslandParams& params) {
  if (params.resolution < 2) {
    OMEGA_LOG_ERROR("heightmap",
                    "makeProceduralIsland: resolution must be >= 2 (got {})",
                    params.resolution);
    return nullptr;
  }

  const int n = params.resolution;
  std::vector<float> samples(static_cast<size_t>(n) * n);
  const float invN1 = 1.0f / static_cast<float>(n - 1);
  for (int z = 0; z < n; ++z) {
    const float v = static_cast<float>(z) * invN1;
    for (int x = 0; x < n; ++x) {
      const float u = static_cast<float>(x) * invN1;
      samples[static_cast<size_t>(z) * n + x] = proceduralHeightAt(u, v);
    }
  }

  // Centre the grid on world (0, 0) and map the normalized [0,1]² samples
  // onto the requested horizontal span.
  HeightmapTransform transform;
  transform.origin = glm::vec2(-0.5f * params.horizontalExtent,
                               -0.5f * params.horizontalExtent);
  transform.horizontalScale = params.horizontalExtent;
  transform.verticalScale = params.verticalScale;
  transform.verticalOffset = 0.0f;

  auto heightmap =
      std::make_shared<Heightmap>(std::move(samples), n, n, transform);

  // Optional blur pre-pass: smooths out the highest-frequency detail octave
  // so the surface reads as rolling hills. Blur is cheap at bake time and
  // never runs per-frame.
  if (params.blurRadius >= kMinBlurRadius) {
    heightmap->gaussianBlur(params.blurRadius);
  }
  return heightmap;
}

// No-arg convenience overload. See the header comment on the two-overload
// split: we can't spell this as `= {}` on the declaration because C++ won't
// let a nested-class aggregate initializer land as a default argument in the
// same class body. Defining it out-of-line here is fine because
// `ProceduralIslandParams` is fully complete by the point this definition is
// parsed.
std::shared_ptr<Heightmap> Heightmap::makeProceduralIsland() {
  return makeProceduralIsland(ProceduralIslandParams{});
}

int Heightmap::clampX(int x) const {
  if (x < 0) return 0;
  if (x >= width_) return width_ - 1;
  return x;
}

int Heightmap::clampZ(int z) const {
  if (z < 0) return 0;
  if (z >= height_) return height_ - 1;
  return z;
}

float Heightmap::sample(int x, int z) const {
  if (!valid()) return 0.0f;
  return samples_[clampZ(z) * width_ + clampX(x)];
}

float Heightmap::sampleBilinear(float u, float v) const {
  if (!valid()) return 0.0f;
  const float cu = clampUv(u);
  const float cv = clampUv(v);

  const float fx = cu * static_cast<float>(width_ - 1);
  const float fz = cv * static_cast<float>(height_ - 1);

  const int x0 = static_cast<int>(std::floor(fx));
  const int z0 = static_cast<int>(std::floor(fz));
  const int x1 = clampX(x0 + 1);
  const int z1 = clampZ(z0 + 1);

  const float tx = fx - static_cast<float>(x0);
  const float tz = fz - static_cast<float>(z0);

  const float h00 = sample(x0, z0);
  const float h10 = sample(x1, z0);
  const float h01 = sample(x0, z1);
  const float h11 = sample(x1, z1);

  const float a = h00 * (1.0f - tx) + h10 * tx;
  const float b = h01 * (1.0f - tx) + h11 * tx;
  return a * (1.0f - tz) + b * tz;
}

float Heightmap::cubic(float v0, float v1, float v2, float v3, float t) {
  // Catmull-Rom: smooth through v1 and v2, uses v0/v3 for tangent estimation.
  // Reduces to linear interpolation when all four values are collinear.
  const float t2 = t * t;
  const float t3 = t2 * t;
  return 0.5f * ((2.0f * v1) +
                 (-v0 + v2) * t +
                 (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
                 (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3);
}

float Heightmap::sampleBicubic(float u, float v) const {
  if (!valid()) return 0.0f;
  const float cu = clampUv(u);
  const float cv = clampUv(v);

  const float fx = cu * static_cast<float>(width_ - 1);
  const float fz = cv * static_cast<float>(height_ - 1);

  const int x1 = static_cast<int>(std::floor(fx));
  const int z1 = static_cast<int>(std::floor(fz));
  const float tx = fx - static_cast<float>(x1);
  const float tz = fz - static_cast<float>(z1);

  // Fetch one row of four cubic taps. Outside-the-grid taps are linearly
  // extrapolated from the two nearest in-bounds samples rather than clamped.
  // Clamping at the edge turns the outer tap into a duplicate of the edge
  // sample, which destroys Catmull-Rom's ability to reproduce linear
  // functions near the boundary. Linear extrapolation keeps that property.
  auto rowTaps = [this](int zz, int x1) -> std::array<float, 4> {
    const float s0 = sample(clampX(x1 + 0), zz);
    const float s1 = sample(clampX(x1 + 1), zz);
    const float vm1 = (x1 - 1 >= 0)
                          ? sample(x1 - 1, zz)
                          : 2.0f * s0 - s1;
    const float v2  = (x1 + 2 < width_)
                          ? sample(x1 + 2, zz)
                          : 2.0f * s1 - s0;
    return {vm1, s0, s1, v2};
  };

  // Same idea vertically: for the four bicubic row-indices {z1-1, z1, z1+1, z1+2},
  // synthesize out-of-range rows by linear extrapolation of the in-range ones.
  auto rowValueAt = [&](int j) -> float {
    const int zz = z1 + j;
    if (zz >= 0 && zz < height_) {
      auto taps = rowTaps(zz, x1);
      return cubic(taps[0], taps[1], taps[2], taps[3], tx);
    }
    // Extrapolate using the two nearest valid rows.
    const int zA = (zz < 0) ? 0 : height_ - 1;
    const int zB = (zz < 0) ? 1 : height_ - 2;
    auto tapsA = rowTaps(zA, x1);
    auto tapsB = rowTaps(zB, x1);
    const float a = cubic(tapsA[0], tapsA[1], tapsA[2], tapsA[3], tx);
    const float b = cubic(tapsB[0], tapsB[1], tapsB[2], tapsB[3], tx);
    return 2.0f * a - b;
  };

  const float rA = rowValueAt(-1);
  const float rB = rowValueAt( 0);
  const float rC = rowValueAt( 1);
  const float rD = rowValueAt( 2);
  return cubic(rA, rB, rC, rD, tz);
}

glm::vec2 Heightmap::gradientWorld(float u, float v) const {
  if (!valid()) return glm::vec2(0.0f);

  // Central differences in UV with a one-texel offset, converted to world
  // gradient by dividing by the world-space distance between those taps.
  const float dxUv = 1.0f / static_cast<float>(width_ - 1);
  const float dzUv = 1.0f / static_cast<float>(height_ - 1);
  const float dxWorld = dxUv * transform_.horizontalScale;
  const float dzWorld = dzUv * transform_.horizontalScale;

  const float hL = sampleBilinear(u - dxUv, v) * transform_.verticalScale;
  const float hR = sampleBilinear(u + dxUv, v) * transform_.verticalScale;
  const float hD = sampleBilinear(u, v - dzUv) * transform_.verticalScale;
  const float hU = sampleBilinear(u, v + dzUv) * transform_.verticalScale;

  // Guard against degenerate transforms.
  const float invDx = (dxWorld > 0.0f) ? 1.0f / (2.0f * dxWorld) : 0.0f;
  const float invDz = (dzWorld > 0.0f) ? 1.0f / (2.0f * dzWorld) : 0.0f;

  return glm::vec2((hR - hL) * invDx, (hU - hD) * invDz);
}

glm::vec3 Heightmap::sampleNormal(float u, float v) const {
  if (!valid()) return glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec2 g = gradientWorld(u, v);
  // Surface is y = H(x, z); the outward unit normal is (-dH/dx, 1, -dH/dz).
  glm::vec3 n(-g.x, 1.0f, -g.y);
  const float len = glm::length(n);
  if (len > 0.0f) n /= len;
  return n;
}

float Heightmap::sampleSlope(float u, float v) const {
  const glm::vec3 n = sampleNormal(u, v);
  return 1.0f - n.y;
}

glm::vec2 Heightmap::worldToUv(float worldX, float worldZ) const {
  if (transform_.horizontalScale <= 0.0f) return glm::vec2(0.0f);
  const float u = (worldX - transform_.origin.x) / transform_.horizontalScale;
  const float v = (worldZ - transform_.origin.y) / transform_.horizontalScale;
  return glm::vec2(u, v);
}

float Heightmap::heightAtWorld(float worldX, float worldZ) const {
  if (!valid()) return transform_.verticalOffset;
  const glm::vec2 uv = worldToUv(worldX, worldZ);
  return sampleBilinear(uv.x, uv.y) * transform_.verticalScale +
         transform_.verticalOffset;
}

glm::vec3 Heightmap::normalAtWorld(float worldX, float worldZ) const {
  if (!valid()) return glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec2 uv = worldToUv(worldX, worldZ);
  return sampleNormal(uv.x, uv.y);
}

void Heightmap::gaussianBlur(int radius) {
  if (!valid()) return;
  if (radius < kMinBlurRadius) radius = kMinBlurRadius;

  // Precompute a 1D Gaussian kernel of size (2*radius + 1). σ = radius / 2
  // gives ~95% of the weight inside the kernel, which is the usual rule of
  // thumb. Weights are normalized so a constant field remains constant.
  const int kernelSize = 2 * radius + 1;
  std::vector<float> kernel(kernelSize);
  const float sigma = static_cast<float>(radius) * 0.5f;
  const float twoSigmaSq = 2.0f * sigma * sigma;
  float sum = 0.0f;
  for (int i = -radius; i <= radius; ++i) {
    const float w = std::exp(-static_cast<float>(i * i) / twoSigmaSq);
    kernel[i + radius] = w;
    sum += w;
  }
  const float invSum = 1.0f / sum;
  for (auto& w : kernel) w *= invSum;

  // Horizontal pass → tmp.
  std::vector<float> tmp(samples_.size());
  for (int z = 0; z < height_; ++z) {
    for (int x = 0; x < width_; ++x) {
      float acc = 0.0f;
      for (int k = -radius; k <= radius; ++k) {
        const int xs = clampX(x + k);
        acc += kernel[k + radius] * samples_[z * width_ + xs];
      }
      tmp[z * width_ + x] = acc;
    }
  }

  // Vertical pass → samples_.
  for (int z = 0; z < height_; ++z) {
    for (int x = 0; x < width_; ++x) {
      float acc = 0.0f;
      for (int k = -radius; k <= radius; ++k) {
        const int zs = clampZ(z + k);
        acc += kernel[k + radius] * tmp[zs * width_ + x];
      }
      samples_[z * width_ + x] = acc;
    }
  }
}

}  // namespace world
}  // namespace omega
