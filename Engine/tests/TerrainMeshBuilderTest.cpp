// TerrainMeshBuilderTest — covers the CPU mesh bake for the single-chunk
// Phase 1 terrain. Pure math: no OpenGL, no file IO. Exercises:
//   - resolution handling (auto, explicit, < 2)
//   - vertex positions respect the HeightmapTransform (origin, scales)
//   - corner vertices sit exactly at the world-space AABB corners
//   - UV tiling
//   - bicubic-vs-bilinear bake produces the same flat mesh on a flat heightmap
//   - normals bake from Heightmap::sampleNormal (≈ +Y on flat terrain)
//   - index buffer is consistent: exactly 6 * (W-1) * (H-1), CCW-oriented
//   - invalid heightmap → empty mesh, no crash

#include <gtest/gtest.h>

#include <cmath>
#include <unordered_set>
#include <vector>

#include <world/Heightmap.h>
#include <world/TerrainMeshBuilder.h>

#include <glm/glm.hpp>

using omega::world::Heightmap;
using omega::world::HeightmapTransform;
using omega::world::TerrainMeshBuilder;
using omega::world::TerrainMeshParams;
using omega::world::TerrainMeshData;

namespace {

constexpr float kEps = 1e-4f;
constexpr float kLooseEps = 1e-3f;

Heightmap makeFlat(int w, int h, float v = 0.5f, HeightmapTransform t = {}) {
  return Heightmap(std::vector<float>(static_cast<size_t>(w) * h, v), w, h, t);
}

Heightmap makeXRamp(int w, int h, HeightmapTransform t = {}) {
  std::vector<float> s(static_cast<size_t>(w) * h);
  const float denom = static_cast<float>(w - 1);
  for (int z = 0; z < h; ++z)
    for (int x = 0; x < w; ++x)
      s[z * w + x] = static_cast<float>(x) / denom;
  return Heightmap(std::move(s), w, h, t);
}

}  // namespace

TEST(TerrainMeshBuilderTest, InvalidHeightmapProducesEmptyMesh) {
  Heightmap bad;  // default-constructed, invalid
  auto data = TerrainMeshBuilder::build(bad);
  EXPECT_TRUE(data.vertices.empty());
  EXPECT_TRUE(data.indices.empty());
}

TEST(TerrainMeshBuilderTest, DefaultResolutionMatchesHeightmap) {
  HeightmapTransform t;  // identity
  auto hm = makeFlat(8, 8, 0.0f, t);
  TerrainMeshParams p;  // resolution == 0 → match heightmap
  auto data = TerrainMeshBuilder::build(hm, p);
  EXPECT_EQ(data.vertices.size(), 8u * 8u);
  EXPECT_EQ(data.indices.size(), 6u * 7u * 7u);
}

TEST(TerrainMeshBuilderTest, ExplicitResolutionOverridesHeightmap) {
  auto hm = makeFlat(32, 32);
  TerrainMeshParams p; p.resolution = 5;
  auto data = TerrainMeshBuilder::build(hm, p);
  EXPECT_EQ(data.vertices.size(), 5u * 5u);
  EXPECT_EQ(data.indices.size(), 6u * 4u * 4u);
}

TEST(TerrainMeshBuilderTest, ResolutionClampedToMinimumTwo) {
  auto hm = makeFlat(8, 8);
  TerrainMeshParams p; p.resolution = 1;  // below the minimum grid
  auto data = TerrainMeshBuilder::build(hm, p);
  EXPECT_EQ(data.vertices.size(), 2u * 2u);
  EXPECT_EQ(data.indices.size(), 6u);
}

TEST(TerrainMeshBuilderTest, WorldPositionsSpanTheTransform) {
  HeightmapTransform t;
  t.origin = glm::vec2(100.0f, 200.0f);
  t.horizontalScale = 50.0f;
  t.verticalScale = 20.0f;
  t.verticalOffset = 5.0f;
  auto hm = makeXRamp(8, 8, t);
  TerrainMeshParams p; p.useBicubic = false;  // bilinear for simple ramp
  auto data = TerrainMeshBuilder::build(hm, p);

  // First vertex must land at (origin.x, verticalOffset, origin.y) — u=v=0,
  // height=0. Last vertex at (origin.x + H, verticalOffset + V, origin.y + H),
  // where H = horizontalScale and V = verticalScale (ramp max = 1).
  const auto& v0 = data.vertices.front();
  EXPECT_NEAR(v0.position.x, 100.0f, kEps);
  EXPECT_NEAR(v0.position.y,   5.0f, kEps);
  EXPECT_NEAR(v0.position.z, 200.0f, kEps);

  const auto& vLast = data.vertices.back();
  EXPECT_NEAR(vLast.position.x, 150.0f, kEps);
  EXPECT_NEAR(vLast.position.y,  25.0f, kEps);
  EXPECT_NEAR(vLast.position.z, 250.0f, kEps);
}

TEST(TerrainMeshBuilderTest, AABBMatchesCornerVertices) {
  HeightmapTransform t;
  t.origin = glm::vec2(-10.0f, -5.0f);
  t.horizontalScale = 20.0f;
  t.verticalScale = 4.0f;
  auto hm = makeXRamp(8, 8, t);
  auto data = TerrainMeshBuilder::build(hm);

  EXPECT_NEAR(data.minBound.x, -10.0f, kEps);
  EXPECT_NEAR(data.minBound.z,  -5.0f, kEps);
  EXPECT_NEAR(data.maxBound.x,  10.0f, kEps);
  EXPECT_NEAR(data.maxBound.z,  15.0f, kEps);
  // Y range [0, verticalScale] because verticalOffset=0 and ramp ∈ [0,1].
  EXPECT_NEAR(data.minBound.y, 0.0f, kEps);
  EXPECT_NEAR(data.maxBound.y, 4.0f, kEps);
}

TEST(TerrainMeshBuilderTest, UvsTileAcrossTerrain) {
  auto hm = makeFlat(8, 8);
  TerrainMeshParams p; p.uvTiling = 4.0f;
  auto data = TerrainMeshBuilder::build(hm, p);

  // First vertex at (0, 0), last at (uvTiling, uvTiling).
  EXPECT_NEAR(data.vertices.front().uv.x, 0.0f, kEps);
  EXPECT_NEAR(data.vertices.front().uv.y, 0.0f, kEps);
  EXPECT_NEAR(data.vertices.back().uv.x, 4.0f, kEps);
  EXPECT_NEAR(data.vertices.back().uv.y, 4.0f, kEps);
}

TEST(TerrainMeshBuilderTest, FlatHeightmapYieldsUpwardNormals) {
  auto hm = makeFlat(8, 8, 0.3f);
  auto data = TerrainMeshBuilder::build(hm);
  for (const auto& v : data.vertices) {
    EXPECT_NEAR(v.normal.x, 0.0f, kEps);
    EXPECT_NEAR(v.normal.y, 1.0f, kEps);
    EXPECT_NEAR(v.normal.z, 0.0f, kEps);
  }
}

TEST(TerrainMeshBuilderTest, RampNormalsTiltInXDirection) {
  HeightmapTransform t; t.horizontalScale = 10.0f; t.verticalScale = 10.0f;
  auto hm = makeXRamp(16, 16, t);
  auto data = TerrainMeshBuilder::build(hm);
  // Pick an interior vertex — avoid the exact boundary where central diffs
  // touch the edge clamp and the normal can drift a bit.
  const auto& v = data.vertices[5 * 16 + 5];
  EXPECT_LT(v.normal.x, -0.1f);
  EXPECT_GT(v.normal.y,  0.0f);
  EXPECT_NEAR(v.normal.z, 0.0f, kLooseEps);
  const float len = std::sqrt(v.normal.x * v.normal.x +
                              v.normal.y * v.normal.y +
                              v.normal.z * v.normal.z);
  EXPECT_NEAR(len, 1.0f, kLooseEps);
}

TEST(TerrainMeshBuilderTest, IndexCountMatchesGridTopology) {
  TerrainMeshParams p; p.resolution = 11;
  auto hm = makeFlat(16, 16);
  auto data = TerrainMeshBuilder::build(hm, p);
  EXPECT_EQ(data.indices.size(), 6u * 10u * 10u);
  // Every index must be in range.
  for (auto idx : data.indices) {
    EXPECT_LT(idx, data.vertices.size());
  }
}

TEST(TerrainMeshBuilderTest, TrianglesAreCcwFromAbove) {
  // Flat heightmap so triangles lie in a known horizontal plane; pick one
  // quad and confirm the emitted triangles have normals pointing +Y via the
  // cross product (v1-v0) × (v2-v0).
  auto hm = makeFlat(4, 4, 0.0f);
  auto data = TerrainMeshBuilder::build(hm);
  ASSERT_GE(data.indices.size(), 6u);
  for (size_t tri = 0; tri + 2 < data.indices.size(); tri += 3) {
    const glm::vec3& a = data.vertices[data.indices[tri + 0]].position;
    const glm::vec3& b = data.vertices[data.indices[tri + 1]].position;
    const glm::vec3& c = data.vertices[data.indices[tri + 2]].position;
    const glm::vec3 n = glm::cross(b - a, c - a);
    EXPECT_GT(n.y, 0.0f) << "triangle " << tri << " facing -Y";
  }
}

TEST(TerrainMeshBuilderTest, BicubicBakeMatchesBilinearOnFlat) {
  HeightmapTransform t; t.horizontalScale = 10.0f; t.verticalScale = 10.0f;
  auto hm = makeFlat(8, 8, 0.3f, t);
  TerrainMeshParams pb; pb.useBicubic = false;
  TerrainMeshParams pc; pc.useBicubic = true;
  auto db = TerrainMeshBuilder::build(hm, pb);
  auto dc = TerrainMeshBuilder::build(hm, pc);
  ASSERT_EQ(db.vertices.size(), dc.vertices.size());
  for (size_t i = 0; i < db.vertices.size(); ++i) {
    EXPECT_NEAR(db.vertices[i].position.y, dc.vertices[i].position.y,
                kLooseEps);
  }
}

TEST(TerrainMeshBuilderTest, TangentFramesAreOrthonormalOnFlat) {
  auto hm = makeFlat(6, 6);
  auto data = TerrainMeshBuilder::build(hm);
  for (const auto& v : data.vertices) {
    EXPECT_NEAR(glm::length(v.tangent), 1.0f, kLooseEps);
    EXPECT_NEAR(glm::length(v.bitangent), 1.0f, kLooseEps);
    EXPECT_NEAR(glm::dot(v.tangent, v.normal), 0.0f, kLooseEps);
    EXPECT_NEAR(glm::dot(v.bitangent, v.normal), 0.0f, kLooseEps);
  }
}
