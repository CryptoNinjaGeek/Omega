// HeightmapTest — covers the CPU heightmap sampler used by the outdoor/terrain
// subsystem (Phase 0). Pure-math tests: no OpenGL context, no file IO. Load()
// is exercised implicitly via the in-memory ctor path since stb_image + zip
// resolution requires a real filesystem and is covered by higher-level tests.

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <world/Heightmap.h>

#include <glm/glm.hpp>

using omega::world::Heightmap;
using omega::world::HeightmapTransform;

namespace {

constexpr float kEps = 1e-4f;
constexpr float kLooseEps = 1e-3f;

// Build a heightmap from a width×height grid of floats given row-major
// (z increases per row).
Heightmap makeGrid(std::vector<float> samples, int width, int height,
                   HeightmapTransform t = {}) {
  return Heightmap(std::move(samples), width, height, t);
}

// A ramp that goes 0 → 1 along +X (columns increase), constant along Z.
Heightmap makeXRamp(int width, int height, HeightmapTransform t = {}) {
  std::vector<float> s(static_cast<size_t>(width) * height);
  const float denom = static_cast<float>(width - 1);
  for (int z = 0; z < height; ++z) {
    for (int x = 0; x < width; ++x) {
      s[z * width + x] = static_cast<float>(x) / denom;
    }
  }
  return makeGrid(std::move(s), width, height, t);
}

// A flat heightmap at a given constant height.
Heightmap makeFlat(int width, int height, float value = 0.5f,
                   HeightmapTransform t = {}) {
  std::vector<float> s(static_cast<size_t>(width) * height, value);
  return makeGrid(std::move(s), width, height, t);
}

}  // namespace

// ----- Construction / validity ---------------------------------------------

TEST(HeightmapTest, DefaultConstructedIsInvalid) {
  Heightmap h;
  EXPECT_FALSE(h.valid());
  EXPECT_EQ(h.width(), 0);
  EXPECT_EQ(h.height(), 0);
}

TEST(HeightmapTest, MismatchedSampleCountIsRejected) {
  // Caller claims 4x4 but only supplies 10 samples — should come out invalid.
  Heightmap h(std::vector<float>(10, 0.0f), 4, 4);
  EXPECT_FALSE(h.valid());
}

TEST(HeightmapTest, WellFormedGridIsValid) {
  Heightmap h = makeFlat(4, 4);
  EXPECT_TRUE(h.valid());
  EXPECT_EQ(h.width(), 4);
  EXPECT_EQ(h.height(), 4);
}

// ----- Raw sampling / clamping ---------------------------------------------

TEST(HeightmapTest, SampleClampsOutOfRangeIndices) {
  Heightmap h = makeXRamp(4, 4);
  // Column 0 of the ramp is 0; column 3 is 1.
  EXPECT_NEAR(h.sample(-10, 0), 0.0f, kEps);
  EXPECT_NEAR(h.sample(99, 0), 1.0f, kEps);
  // Z out of range also clamps without changing the value (constant in Z).
  EXPECT_NEAR(h.sample(0, -5), 0.0f, kEps);
  EXPECT_NEAR(h.sample(3, 99), 1.0f, kEps);
}

// ----- Bilinear -------------------------------------------------------------

TEST(HeightmapTest, BilinearMatchesCornerSamples) {
  Heightmap h = makeXRamp(5, 5);
  EXPECT_NEAR(h.sampleBilinear(0.0f, 0.0f), 0.0f, kEps);
  EXPECT_NEAR(h.sampleBilinear(1.0f, 0.0f), 1.0f, kEps);
  EXPECT_NEAR(h.sampleBilinear(0.0f, 1.0f), 0.0f, kEps);
  EXPECT_NEAR(h.sampleBilinear(1.0f, 1.0f), 1.0f, kEps);
}

TEST(HeightmapTest, BilinearInterpolatesMidpoint) {
  Heightmap h = makeXRamp(5, 5);
  // Ramp along X means midpoint U=0.5 → 0.5 at every V.
  EXPECT_NEAR(h.sampleBilinear(0.5f, 0.25f), 0.5f, kEps);
  EXPECT_NEAR(h.sampleBilinear(0.5f, 0.75f), 0.5f, kEps);
  // U=0.25 lies exactly on column 1 of a 5-wide grid → 0.25.
  EXPECT_NEAR(h.sampleBilinear(0.25f, 0.5f), 0.25f, kEps);
}

TEST(HeightmapTest, BilinearClampsOutOfRangeUv) {
  Heightmap h = makeXRamp(5, 5);
  EXPECT_NEAR(h.sampleBilinear(-1.0f, 0.0f), 0.0f, kEps);
  EXPECT_NEAR(h.sampleBilinear(2.0f, 0.0f), 1.0f, kEps);
}

// ----- Bicubic --------------------------------------------------------------

TEST(HeightmapTest, BicubicMatchesCornerSamples) {
  Heightmap h = makeXRamp(8, 8);
  EXPECT_NEAR(h.sampleBicubic(0.0f, 0.0f), 0.0f, kLooseEps);
  EXPECT_NEAR(h.sampleBicubic(1.0f, 0.0f), 1.0f, kLooseEps);
  EXPECT_NEAR(h.sampleBicubic(1.0f, 1.0f), 1.0f, kLooseEps);
}

TEST(HeightmapTest, BicubicOnLinearRampIsLinear) {
  // Catmull-Rom reproduces linear functions exactly on the interior.
  Heightmap h = makeXRamp(8, 8);
  for (float u : {0.1f, 0.25f, 0.4f, 0.6f, 0.75f, 0.9f}) {
    EXPECT_NEAR(h.sampleBicubic(u, 0.5f), u, kLooseEps)
        << "bicubic ramp at u=" << u;
  }
}

// ----- Normals --------------------------------------------------------------

TEST(HeightmapTest, FlatHeightmapHasUpwardNormal) {
  Heightmap h = makeFlat(8, 8, 0.5f);
  const glm::vec3 n = h.sampleNormal(0.5f, 0.5f);
  EXPECT_NEAR(n.x, 0.0f, kEps);
  EXPECT_NEAR(n.y, 1.0f, kEps);
  EXPECT_NEAR(n.z, 0.0f, kEps);
}

TEST(HeightmapTest, RampInXTiltsNormalNegativeX) {
  // Ramp rises along +X → surface faces backward along -X → normal.x < 0.
  HeightmapTransform t;
  t.horizontalScale = 10.0f;
  t.verticalScale = 10.0f;  // comparable scales → clearly tilted normal
  Heightmap h = makeXRamp(8, 8, t);
  const glm::vec3 n = h.sampleNormal(0.5f, 0.5f);
  EXPECT_LT(n.x, -0.1f);
  EXPECT_GT(n.y, 0.0f);
  EXPECT_NEAR(n.z, 0.0f, kLooseEps);
  EXPECT_NEAR(glm::length(n), 1.0f, kLooseEps);
}

TEST(HeightmapTest, HorizontalScaleFlattensSlope) {
  // A wider terrain (bigger horizontalScale) makes the same heightmap look
  // less steep. Normal.y should be closer to 1 than in the narrow case.
  HeightmapTransform narrow;
  narrow.horizontalScale = 1.0f;
  narrow.verticalScale = 1.0f;
  HeightmapTransform wide = narrow;
  wide.horizontalScale = 100.0f;

  Heightmap hNarrow = makeXRamp(16, 16, narrow);
  Heightmap hWide = makeXRamp(16, 16, wide);

  EXPECT_LT(hNarrow.sampleNormal(0.5f, 0.5f).y,
            hWide.sampleNormal(0.5f, 0.5f).y);
}

// ----- Slope ---------------------------------------------------------------

TEST(HeightmapTest, SlopeIsZeroOnFlat) {
  Heightmap h = makeFlat(8, 8);
  EXPECT_NEAR(h.sampleSlope(0.3f, 0.7f), 0.0f, kEps);
}

TEST(HeightmapTest, SlopeIsPositiveOnRamp) {
  HeightmapTransform t;
  t.horizontalScale = 1.0f;
  t.verticalScale = 1.0f;
  Heightmap h = makeXRamp(8, 8, t);
  const float s = h.sampleSlope(0.5f, 0.5f);
  EXPECT_GT(s, 0.1f);
  EXPECT_LE(s, 1.0f);
}

// ----- World-space helpers --------------------------------------------------

TEST(HeightmapTest, HeightAtWorldAppliesTransform) {
  HeightmapTransform t;
  t.origin = glm::vec2(100.0f, 200.0f);
  t.horizontalScale = 50.0f;
  t.verticalScale = 20.0f;
  t.verticalOffset = 5.0f;

  Heightmap h = makeXRamp(5, 5, t);

  // U=0 corner → h=0 → worldY = 0*20 + 5 = 5.
  EXPECT_NEAR(h.heightAtWorld(100.0f, 200.0f), 5.0f, kEps);
  // Far +X corner → h=1 → worldY = 1*20 + 5 = 25.
  EXPECT_NEAR(h.heightAtWorld(150.0f, 200.0f), 25.0f, kEps);
  // Midpoint → h=0.5 → worldY = 15.
  EXPECT_NEAR(h.heightAtWorld(125.0f, 225.0f), 15.0f, kEps);
}

TEST(HeightmapTest, HeightAtWorldClampsOutsideTerrain) {
  HeightmapTransform t;
  t.horizontalScale = 10.0f;
  t.verticalScale = 10.0f;
  Heightmap h = makeXRamp(5, 5, t);
  // Far outside to the +X: should clamp to the max column height = 10.
  EXPECT_NEAR(h.heightAtWorld(1000.0f, 0.0f), 10.0f, kEps);
  // Far outside to the -X: should clamp to 0.
  EXPECT_NEAR(h.heightAtWorld(-1000.0f, 0.0f), 0.0f, kEps);
}

// ----- Gaussian blur --------------------------------------------------------

TEST(HeightmapTest, GaussianBlurPreservesConstantField) {
  Heightmap h = makeFlat(8, 8, 0.42f);
  h.gaussianBlur(2);
  for (int z = 0; z < h.height(); ++z) {
    for (int x = 0; x < h.width(); ++x) {
      EXPECT_NEAR(h.sample(x, z), 0.42f, kLooseEps)
          << "blur altered constant field at (" << x << "," << z << ")";
    }
  }
}

TEST(HeightmapTest, GaussianBlurDampensSpike) {
  // 9x9 field with a single spike at the center. After blurring, the spike's
  // peak must drop and its immediate neighbors must rise.
  const int n = 9;
  std::vector<float> s(static_cast<size_t>(n) * n, 0.0f);
  s[4 * n + 4] = 1.0f;
  Heightmap h(s, n, n);
  const float peakBefore = h.sample(4, 4);
  const float neighborBefore = h.sample(5, 4);
  h.gaussianBlur(2);
  const float peakAfter = h.sample(4, 4);
  const float neighborAfter = h.sample(5, 4);
  EXPECT_LT(peakAfter, peakBefore);
  EXPECT_GT(neighborAfter, neighborBefore);
}

TEST(HeightmapTest, GaussianBlurClampsSmallRadius) {
  // radius 0 is illegal; should be clamped up and still produce a valid field
  // (we don't require it to match a specific value, just that it runs and
  // leaves the heightmap valid and non-nan).
  Heightmap h = makeXRamp(5, 5);
  h.gaussianBlur(0);
  EXPECT_TRUE(h.valid());
  for (int z = 0; z < h.height(); ++z) {
    for (int x = 0; x < h.width(); ++x) {
      const float v = h.sample(x, z);
      EXPECT_TRUE(std::isfinite(v));
    }
  }
}
