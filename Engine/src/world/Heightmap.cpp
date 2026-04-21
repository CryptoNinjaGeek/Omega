#include <world/Heightmap.h>

#include <algorithm>
#include <array>
#include <cmath>

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
