// TerrainCameraControllerTest — covers the analytical ground-follow and
// horizontal-slope filter used by the Phase 1 Outdoor demo. Pure math: no
// real Camera is exercised here; instead we test `resolvePosition`,
// `groundHeight`, `isWalkable`, and `filterHorizontal` directly.

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

#include <world/Heightmap.h>
#include <world/TerrainCameraController.h>

#include <glm/glm.hpp>

using omega::world::Heightmap;
using omega::world::HeightmapTransform;
using omega::world::TerrainCameraController;
using omega::world::TerrainCameraParams;

namespace {

constexpr float kEps = 1e-4f;
constexpr float kLooseEps = 1e-3f;

std::shared_ptr<Heightmap> makeFlatHm(int w, int h, float v,
                                      HeightmapTransform t = {}) {
  return std::make_shared<Heightmap>(
      std::vector<float>(static_cast<size_t>(w) * h, v), w, h, t);
}

// Ramp heightmap: samples[x,z] = x / (w-1).
std::shared_ptr<Heightmap> makeXRampHm(int w, int h,
                                       HeightmapTransform t = {}) {
  std::vector<float> s(static_cast<size_t>(w) * h);
  const float denom = static_cast<float>(w - 1);
  for (int z = 0; z < h; ++z)
    for (int x = 0; x < w; ++x)
      s[z * w + x] = static_cast<float>(x) / denom;
  return std::make_shared<Heightmap>(std::move(s), w, h, t);
}

}  // namespace

TEST(TerrainCameraControllerTest, UnboundControllerDoesNotCrash) {
  TerrainCameraController c;  // no heightmap bound
  // groundHeight should return a finite zero (sea-level fallback). isWalkable
  // must return true (nothing to block against).
  EXPECT_NEAR(c.groundHeight(glm::vec2(0.0f, 0.0f)), 0.0f, kEps);
  EXPECT_TRUE(c.isWalkable(glm::vec2(10.0f, 20.0f)));
  // resolvePosition must produce the input XZ back with a finite Y.
  auto p = c.resolvePosition(glm::vec3(1, 2, 3), glm::vec2(4, 5), 0.016f);
  EXPECT_NEAR(p.x, 4.0f, kEps);
  EXPECT_NEAR(p.z, 5.0f, kEps);
  EXPECT_TRUE(std::isfinite(p.y));
}

TEST(TerrainCameraControllerTest, GroundHeightRespectsTransform) {
  HeightmapTransform t;
  t.origin = glm::vec2(10.0f, 20.0f);
  t.horizontalScale = 50.0f;
  t.verticalScale   = 4.0f;
  t.verticalOffset  = 1.0f;
  auto hm = makeXRampHm(8, 8, t);
  TerrainCameraController c(hm);
  // At the origin corner: ramp=0 → y = verticalOffset = 1.
  EXPECT_NEAR(c.groundHeight(glm::vec2(10.0f, 20.0f)), 1.0f, kEps);
  // At the far corner: ramp=1 → y = verticalOffset + verticalScale = 5.
  EXPECT_NEAR(c.groundHeight(glm::vec2(60.0f, 70.0f)), 5.0f, kEps);
  // Outside the terrain: clamped, so matches the far-X edge ground.
  EXPECT_NEAR(c.groundHeight(glm::vec2(500.0f, 20.0f)), 5.0f, kEps);
}

TEST(TerrainCameraControllerTest, NoSmoothingSnapsExactly) {
  auto hm = makeFlatHm(8, 8, 0.25f, [] {
    HeightmapTransform t; t.verticalScale = 4.0f; return t;
  }());
  TerrainCameraParams p; p.eyeHeight = 1.5f; p.groundSmoothing = 0.0f;
  p.stepHeight = 100.0f;  // large so clamping doesn't bite
  TerrainCameraController c(hm, p);
  // Flat heightmap at 0.25 * 4 = 1.0 (world); + eye = 2.5.
  auto out = c.resolvePosition(glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec2(1.0f, 2.0f), 0.016f);
  EXPECT_NEAR(out.y, 2.5f, kEps);
  EXPECT_NEAR(out.x, 1.0f, kEps);
  EXPECT_NEAR(out.z, 2.0f, kEps);
}

TEST(TerrainCameraControllerTest, SmoothingTakesMultipleFrames) {
  auto hm = makeFlatHm(8, 8, 0.5f);  // ground y = 0.5
  TerrainCameraParams p;
  p.eyeHeight = 0.0f;         // target = ground
  p.groundSmoothing = 10.0f;  // ~70 ms half-life
  p.stepHeight = 100.0f;
  TerrainCameraController c(hm, p);

  glm::vec3 pos(0.0f, 5.0f, 0.0f);  // start 4.5 above the target
  // After one 16 ms frame the error should have dropped, but not vanished.
  auto after1 = c.resolvePosition(pos, glm::vec2(0.0f, 0.0f), 0.016f);
  EXPECT_LT(after1.y, pos.y);
  EXPECT_GT(after1.y, 0.5f);
  // After many frames, we should converge very close to target.
  glm::vec3 step = pos;
  for (int i = 0; i < 200; ++i) {
    step = c.resolvePosition(step, glm::vec2(0.0f, 0.0f), 0.016f);
  }
  EXPECT_NEAR(step.y, 0.5f, kLooseEps);
}

TEST(TerrainCameraControllerTest, StepHeightClampsLargeVerticalDelta) {
  auto hm = makeFlatHm(8, 8, 1.0f);  // ground y = 1
  TerrainCameraParams p;
  p.eyeHeight = 0.0f;
  p.groundSmoothing = 0.0f;  // snap — so smoothing doesn't hide the clamp
  p.stepHeight = 0.2f;
  TerrainCameraController c(hm, p);
  // Trying to drop from y=5 to y=1 in one frame should be clamped to y=4.8.
  auto out = c.resolvePosition(glm::vec3(0, 5, 0), glm::vec2(0, 0), 1.0f);
  EXPECT_NEAR(out.y, 4.8f, kEps);
}

TEST(TerrainCameraControllerTest, WalkableOnFlatTerrain) {
  auto hm = makeFlatHm(8, 8, 0.5f);
  TerrainCameraController c(hm);
  EXPECT_TRUE(c.isWalkable(glm::vec2(1.0f, 1.0f)));
  EXPECT_TRUE(c.isWalkable(glm::vec2(-1000.0f, 1000.0f)));  // clamped
}

TEST(TerrainCameraControllerTest, SteepRampBlocksMotionAboveThreshold) {
  // Make a ramp steep enough that slope exceeds the default maxWalkableSlope.
  HeightmapTransform t; t.horizontalScale = 1.0f; t.verticalScale = 10.0f;
  auto hm = makeXRampHm(8, 8, t);
  TerrainCameraController c(hm);
  // Threshold ~ 0.7; a 10× vertical/1× horizontal ramp is brutally steep.
  EXPECT_FALSE(c.isWalkable(glm::vec2(0.5f, 0.5f)));
  // Filter should reject the move.
  auto blocked = c.filterHorizontal(glm::vec2(0.0f, 0.0f),
                                    glm::vec2(0.5f, 0.5f));
  EXPECT_NEAR(blocked.x, 0.0f, kEps);
  EXPECT_NEAR(blocked.y, 0.0f, kEps);
}

TEST(TerrainCameraControllerTest, GentleRampAllowsMotion) {
  HeightmapTransform t; t.horizontalScale = 200.0f; t.verticalScale = 5.0f;
  auto hm = makeXRampHm(8, 8, t);
  TerrainCameraController c(hm);
  EXPECT_TRUE(c.isWalkable(glm::vec2(50.0f, 50.0f)));
  auto ok = c.filterHorizontal(glm::vec2(40.0f, 40.0f),
                               glm::vec2(50.0f, 50.0f));
  EXPECT_NEAR(ok.x, 50.0f, kEps);
  EXPECT_NEAR(ok.y, 50.0f, kEps);
}

TEST(TerrainCameraControllerTest, FilterHorizontalNoOpsWithoutHeightmap) {
  TerrainCameraController c;  // unbound
  auto out = c.filterHorizontal(glm::vec2(1, 2), glm::vec2(3, 4));
  EXPECT_NEAR(out.x, 3.0f, kEps);
  EXPECT_NEAR(out.y, 4.0f, kEps);
}

TEST(TerrainCameraControllerTest, UnwalkableDestinationReturnsPreviousXZ) {
  HeightmapTransform t; t.horizontalScale = 1.0f; t.verticalScale = 10.0f;
  auto hm = makeXRampHm(8, 8, t);
  TerrainCameraController c(hm);
  TerrainCameraParams p;  // default eyeHeight 1.7, smoothing 10
  p.groundSmoothing = 0.0f;
  p.stepHeight = 100.0f;
  c.setParams(p);
  auto out = c.resolvePosition(glm::vec3(0.0f, 5.0f, 0.0f),
                               glm::vec2(0.5f, 0.5f), 0.016f);
  // Steep ramp blocks horizontal; XZ should snap back to previous.
  EXPECT_NEAR(out.x, 0.0f, kEps);
  EXPECT_NEAR(out.z, 0.0f, kEps);
}
