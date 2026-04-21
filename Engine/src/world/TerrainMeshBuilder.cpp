#include <world/TerrainMeshBuilder.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <world/Heightmap.h>

namespace omega {
namespace world {

namespace {

// Clamp a requested resolution into a range that actually produces triangles.
// 2 is the minimum that gives one quad; anything less is a no-op.
int resolveResolution(int requested, int heightmapSide) {
  if (requested <= 0) return heightmapSide;
  if (requested < 2) return 2;
  return requested;
}

}  // namespace

TerrainMeshData TerrainMeshBuilder::build(const Heightmap& heightmap,
                                          const TerrainMeshParams& params) {
  TerrainMeshData out;
  if (!heightmap.valid()) {
    return out;
  }

  // Prefer a square grid matching the heightmap's shorter side when the caller
  // asked for "auto" resolution, so non-square heightmaps still produce a
  // well-proportioned mesh rather than silently stretching one axis.
  const int hmSide = std::min(heightmap.width(), heightmap.height());
  const int resolution = resolveResolution(params.resolution, hmSide);

  const int W = resolution;  // columns along X
  const int H = resolution;  // rows along Z

  const auto& t = heightmap.transform();

  out.vertices.resize(static_cast<size_t>(W) * H);

  // Track world-space AABB on the fly — the caller usually wants it for
  // bounding sphere + camera spawn height + fog tuning. Cheaper to compute
  // here than to loop again over the vertex buffer.
  glm::vec3 mn( std::numeric_limits<float>::max());
  glm::vec3 mx(-std::numeric_limits<float>::max());

  // Per-vertex bake: UV = grid position in [0,1], world XZ = transform, Y =
  // heightmap height (bicubic or bilinear), normal = world-space normal from
  // heightmap gradient (already respects horizontal/vertical scale).
  const float invWm1 = (W > 1) ? 1.0f / static_cast<float>(W - 1) : 0.0f;
  const float invHm1 = (H > 1) ? 1.0f / static_cast<float>(H - 1) : 0.0f;

  for (int z = 0; z < H; ++z) {
    for (int x = 0; x < W; ++x) {
      const float u = static_cast<float>(x) * invWm1;
      const float v = static_cast<float>(z) * invHm1;

      const float hNorm = params.useBicubic ? heightmap.sampleBicubic(u, v)
                                            : heightmap.sampleBilinear(u, v);
      const float worldY = hNorm * t.verticalScale + t.verticalOffset;
      const float worldX = t.origin.x + u * t.horizontalScale;
      const float worldZ = t.origin.y + v * t.horizontalScale;

      // Normal comes from gradientWorld() which central-differences a one-
      // texel offset, so it approximates the surface slope even when the
      // bake is down-sampled (resolution < heightmap size).
      const glm::vec3 normal = heightmap.sampleNormal(u, v);

      // Tangent/bitangent derived from the normal: if n = (-dh/dx, 1, -dh/dz),
      // then (1, dh/dx, 0) and (0, dh/dz, 1) are in-plane. We normalize after
      // reconstructing because degenerate vertical normals (n.y ≈ 0) would
      // blow the divisions up — in that case fall back to world axes.
      glm::vec3 tangent(1.0f, 0.0f, 0.0f);
      glm::vec3 bitangent(0.0f, 0.0f, 1.0f);
      if (std::fabs(normal.y) > 1e-4f) {
        const float dHdx = -normal.x / normal.y;
        const float dHdz = -normal.z / normal.y;
        tangent = glm::vec3(1.0f, dHdx, 0.0f);
        bitangent = glm::vec3(0.0f, dHdz, 1.0f);
        const float lt = glm::length(tangent);
        const float lb = glm::length(bitangent);
        if (lt > 0.0f) tangent /= lt;
        if (lb > 0.0f) bitangent /= lb;
      }

      geometry::Vertex vx;
      vx.position  = glm::vec3(worldX, worldY, worldZ);
      vx.normal    = normal;
      vx.uv        = glm::vec2(u * params.uvTiling, v * params.uvTiling);
      vx.tangent   = tangent;
      vx.bitangent = bitangent;
      out.vertices[static_cast<size_t>(z) * W + x] = vx;

      mn = glm::min(mn, vx.position);
      mx = glm::max(mx, vx.position);
    }
  }

  out.minBound = mn;
  out.maxBound = mx;

  // Two triangles per cell, CCW as seen from above. The engine's shaders
  // don't flip winding, so this matches the existing Plane generator.
  out.indices.reserve(static_cast<size_t>((W - 1)) * (H - 1) * 6);
  for (int z = 0; z < H - 1; ++z) {
    for (int x = 0; x < W - 1; ++x) {
      const unsigned int i00 = static_cast<unsigned int>(z      * W + x);
      const unsigned int i10 = static_cast<unsigned int>(z      * W + x + 1);
      const unsigned int i01 = static_cast<unsigned int>((z + 1) * W + x);
      const unsigned int i11 = static_cast<unsigned int>((z + 1) * W + x + 1);
      // Triangle 1: i00 -> i01 -> i10  (CCW when viewed from +Y)
      out.indices.push_back(i00);
      out.indices.push_back(i01);
      out.indices.push_back(i10);
      // Triangle 2: i10 -> i01 -> i11
      out.indices.push_back(i10);
      out.indices.push_back(i01);
      out.indices.push_back(i11);
    }
  }

  return out;
}

}  // namespace world
}  // namespace omega
