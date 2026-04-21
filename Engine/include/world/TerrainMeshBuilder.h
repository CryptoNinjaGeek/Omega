#pragma once

// TerrainMeshBuilder — turns a Heightmap into a regular grid of vertices and
// triangle indices usable as an `Object`. No OpenGL calls happen here: the
// builder returns plain Vertex/index buffers that the caller can hand to
// `ObjectGenerator::mesh(...)` for VAO/VBO upload.
//
// Splitting "build the CPU data" from "upload to GL" is deliberate:
//   - We can unit-test the mesh layout (vertex count, UV range, world-space
//     position placement, normals matching the Heightmap) without a GL
//     context.
//   - Phase 3 (CDLOD) will replace the full-grid build with per-node grids
//     drawn from a shared unit VBO, but the UV/world math stays the same;
//     encapsulating it here keeps that swap local.

#include <memory>
#include <string>
#include <vector>

#include <system/Global.h>
#include <geometry/Vertex.h>

#include <glm/glm.hpp>

namespace omega {
namespace world {

class Heightmap;

// Tunables for one terrain bake.
struct TerrainMeshParams {
  // Number of vertices along one edge of the grid. 0 means "match the
  // heightmap's own resolution", which is the densest possible bake and the
  // right default for small/medium heightmaps. Caller can down-sample for
  // perf testing by forcing a smaller value.
  int resolution{0};

  // How many times the texture tiles across the full terrain span. Higher
  // numbers = crisper detail, lower numbers = broader vistas. 32 is a
  // reasonable default for a 512×512 heightmap at ~1 km world size.
  float uvTiling{32.0f};

  // Use bicubic height sampling for the bake. Slightly smoother surface at
  // chunk interiors; the CDLOD plan calls for bicubic baking specifically so
  // that chunk boundaries stay C¹-continuous once we add multi-chunk builds.
  bool useBicubic{true};
};

// Raw CPU-side mesh ready to be handed to `ObjectGenerator::mesh(...)`.
// Split out so `world/` doesn't depend on `utils/` — the demo code wraps this
// into a `utils::input::MeshInput` at the call site.
struct TerrainMeshData {
  std::vector<geometry::Vertex> vertices;
  std::vector<unsigned int> indices;
  // World-space axis-aligned bounds of the baked mesh. Convenient for the
  // caller to configure bounding spheres, fog tuning, camera spawn, etc.
  glm::vec3 minBound{0.0f};
  glm::vec3 maxBound{0.0f};
};

class OMEGA_EXPORT TerrainMeshBuilder {
 public:
  // Build a single grid mesh covering the entire heightmap region. Safe to
  // call on an invalid heightmap — returns an empty TerrainMeshData in that
  // case (caller can still forward to ObjectGenerator, which will produce an
  // empty Object rather than crashing).
  static TerrainMeshData build(const Heightmap& heightmap,
                               const TerrainMeshParams& params = {});
};

}  // namespace world
}  // namespace omega
