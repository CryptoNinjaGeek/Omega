# Outdoor Scene System — Design Plan

**Status:** Phases 0 and 1 complete (2026-04-21)
**Author:** design session 2026-04-21
**Scope:** heightmap-driven outdoor terrain with CDLOD, analytical collision, sun + CSM shadows, reflective water, sky/fog, and vegetation scatter.

## Progress tracker

- [x] **Phase 0** — Heightmap foundation (done 2026-04-21)
- [x] **Phase 1** — MVP single-chunk terrain (done 2026-04-21)
- [ ] **Phase 2** — Sky and fog
- [ ] **Phase 3** — CDLOD
- [ ] **Phase 4** — Slope-aware texturing + triplanar + detail noise
- [ ] **Phase 5** — Water
- [ ] **Phase 6** — Sun + CSM
- [ ] **Phase 7** — Vegetation
- [ ] **Phase 8** — Polish

---

## 1. Goals

Give the engine a *world* concept: load a heightmap image, generate a large, smooth, textured landscape that streams out to the horizon, let the camera walk over it, and surround it with the usual outdoor furniture (sky, sun, fog, water, plants). Performance target: stay above 60 FPS on a mid-range GPU at 1080p with a ~4 km × 4 km terrain fully visible.

Non-goals (explicit v1 exclusions): procedural terrain generation from noise, biome/weather systems, destruction, grass-vs-player physics, GPU compute-driven mesh generation. These are all natural follow-ups once the skeleton is working.

## 2. What you described vs. what I'm adding

You asked for heightmap loading, height-based texturing, horizon stretch, smoothing, an optimal spatial structure, and collision. I'm folding in the extras you selected (CDLOD, analytical collision, sky+fog, sun+CSM, water, vegetation) and adding these pieces you didn't mention but that the project will beg for once it exists:

*Slope-aware texturing.* Height alone produces visibly fake terrain — grass climbing vertical cliffs, snow on a flat beach if you get unlucky. Blending by height *and* slope (derived from the heightmap gradient) fixes this. Triplanar projection on steep faces stops textures from stretching.

*Heightmap normals baked from gradient.* Needed for lighting and for the slope calculation. Computed once on load; much higher quality than face-averaged normals.

*Bounding sphere per chunk/node.* Your codebase has a gotcha where objects without bounding spheres silently skip frustum culling (memory note: `project_object_cloner_bounding_sphere.md`). Every terrain node must set one, or the quadtree culling collapses into "draw everything."

*Horizon skirt.* A ring of geometry at the outer CDLOD boundary that drops steeply to the fog color. Hides the terrain edge cleanly.

*Pre-pass Gaussian smoothing of the heightmap.* Your "smoothed out" requirement. Apply once on load with a configurable radius so you can drop in a crunchy 256×256 source and still get a readable landscape.

*Detail noise layer.* One heightmap can't give you both big silhouettes and crisp rock. Blend a tiling high-frequency noise texture into the displacement in the vertex shader for micro-relief, at near-zero cost.

*Bicubic vs. bilinear at different layers.* Use bicubic when baking mesh vertices (C¹ continuity across chunks), bilinear for the analytical collision sampler (cheap, and the camera doesn't need perfect smoothness).

*Debug visualizations.* Wireframe toggle, LOD coloring, normal visualization, quadtree overlay. These pay for themselves the first time something looks wrong.

*Player walk affordances.* Smoothed ground-follow (to avoid jitter at LOD transitions), max walkable slope, step-height tolerance, eye-height offset. Without these the FPS camera feels awful.

*Reuse the portal clipping-plane infrastructure for water reflection.* Scene already pushes clipping planes into shaders — that's exactly what planar reflection needs. Nice bit of leverage.

*Resource strategy for new shaders.* Per `project_resources_zip.md`, `bin/resources.zip` is incomplete and anything that loads `":/shaders/..."` needs a disk fallback plus an `isValid()` check. All new shaders (terrain, water, sky, shadow, grass) must follow that pattern.

*Cache pre-processed data to disk.* Normals, splat weights, and flattened quadtree node list can all be computed once and cached next to the heightmap. Shaves seconds off load.

## 3. Data structures and algorithms

### 3.1 Heightmap (CPU-side authority)

`class Heightmap` owns a `std::vector<float>` of size `N × N` in `[0,1]`. API:

```cpp
float sampleBilinear(float u, float v) const;   // smoothed, for collision / analytical queries
float sampleBicubic (float u, float v) const;   // for mesh bake, continuous derivatives
glm::vec3 sampleNormal(float u, float v) const; // from gradient
float sampleSlope(float u, float v) const;      // dot(normal, up)
void gaussianBlur(int radius);                  // the "smoothed out" pre-pass
```

World-space conversion is done via a `HeightmapTransform { glm::vec2 origin; float horizontalScale; float verticalScale; }`. The heightmap can then be 256² or 512² while representing any world size — horizontal scale decouples source resolution from terrain size.

Loaded through `fs::instance()->data()` + stb_image (grayscale PNG → float, color PNGs fall back to the red channel). This reuses the engine's existing image pipeline, so `":/heightmaps/..."` zip paths and disk fallback both work for free. Optional raw R16 path for higher vertical precision later.

### 3.2 Spatial structure — CDLOD quadtree

Why this is the right answer to "best algorithm for speed": it gives you three wins simultaneously — per-node frustum culling, logarithmic LOD selection by distance, and a shared vertex buffer across every visible node. There's no competing structure that hits all three.

**Node layout.** A `TerrainNode` is a square region of world space defined by `(originX, originZ, size, depth)`. The root covers the whole world; each node has four children covering its quadrants, down to a fixed maximum depth. Each node also stores a min/max height computed from the heightmap region it covers, so culling uses a real AABB, not a loose sphere over flat terrain.

**Shared grid mesh.** One VBO holds a unit grid of `M × M` vertices (typically 17×17 or 33×33, including morph seam). Every visible node draws the same VBO with a per-draw uniform `{origin, size, lodLevel, morphRange}`. The vertex shader transforms unit-grid coords into world space using those uniforms, samples the heightmap texture, and applies CDLOD morph. One VBO, one draw per node, instanced if you want to push further.

**Traversal.** Per-frame recursive descent from the root: if `distance(camera, node.center) > node.size × lodFactor`, draw this node; otherwise recurse into children. Cull nodes whose AABB fails the frustum test — this is where your existing `Frustum` class plugs in. Expected visible node count at any time: ~200–600, which is fine.

**Morph.** Each CDLOD node renders at its own LOD level but morphs its outer vertices toward the coarser parent's positions as the camera approaches the threshold. Morph factor is computed per-vertex in the vertex shader from distance-to-camera — no CPU-side morph bookkeeping. Eliminates popping.

**Pooling.** Node objects are tiny (~64 bytes). Preallocate them in a contiguous array indexed by Morton code so traversal is cache-friendly.

### 3.3 Collision — analytical sampling (no physics body needed)

For the camera and future kinematic characters, ground height comes from `Heightmap::sampleBilinear(worldToUV(pos.xz))`. That's a handful of ALU ops. A `TerrainCameraController` wraps the existing FPS camera and, each frame:

1. Sample ground height `gy` at the camera's `(x, z)`.
2. Set `camera.y = lerp(camera.y, gy + eyeHeight, smoothing)` — smoothed so LOD-driven terrain changes don't produce camera jitter.
3. If `sampleSlope(...)` exceeds `maxWalkableSlope`, block forward motion in that direction (the "can't climb a cliff" rule).
4. Clamp the vertical delta per frame to `stepHeight` so small bumps feel fine and big ones feel like walls.

ReactPhysics3D stays unused for terrain in v1. If you later want crates to roll down hills, add a `HeightFieldShape` on a static body — same heightmap, different consumer.

## 4. Rendering — terrain material

One terrain shader, five textures (sand, grass, dirt, rock, snow), blended by height and slope. In the fragment shader:

- Compute normalized height `h = (worldY - seaLevel) / heightRange`.
- Compute slope `s = 1 - dot(normal, up)`.
- Define weight bands with `smoothstep`. Sketch:
  - Sand: `h ∈ [0, 0.05]`
  - Grass: `h ∈ [0.05, 0.5]`, `s < 0.3`
  - Dirt: transitional band and `s ∈ [0.3, 0.6]`
  - Rock: `s > 0.6` at any height (cliffs are rock regardless of altitude)
  - Snow: `h > 0.8`, `s < 0.5`
- Normalize weights, sample five textures, weighted sum.
- **Triplanar projection** when `s > 0.5`: sample the rock texture three times (XY, XZ, YZ planes), blend by squared world-space normal. Prevents stretched textures on cliffs.
- Add detail noise into the lighting normal for micro-relief.

Lighting uses the existing directional-light path, plus fog applied last.

## 5. Other subsystems

### 5.1 Skybox + fog

Cubemap loaded via libpng into a `GL_TEXTURE_CUBE_MAP`. Rendered last with `depth = 1.0` trick (`gl_Position.z = gl_Position.w` in the vertex shader), so it only fills pixels not written by scene geometry.

Exponential-squared fog in every world-space shader, colored to match the sky horizon. Density tuned so the outermost CDLOD ring fades out before its popping is visible — the fog *is* the cull distance disguise.

### 5.2 Directional sun + cascaded shadow maps

Add a `SunLight` (wraps existing `DirectionalLight`) plus a `ShadowCaster` pass that renders the scene to three cascaded depth textures, each fit to a slice of the camera frustum (near ~20 m, mid ~80 m, far ~300 m for a 4 km terrain).

Cascade fitting: project the main camera's frustum slice corners into light space, compute a tight ortho, snap to texel size to prevent shadow shimmer. 2048² per cascade is a reasonable default.

Receivers: terrain, vegetation, all `Object` instances. Terrain fragment shader samples the appropriate cascade, PCF-filters 3×3 taps.

### 5.3 Water

One horizontal quad at sea level, covering visible area. Two render passes:

- **Reflection pass.** Camera mirrored across the water plane, scene re-rendered to an FBO with your existing clipping plane set to `y = seaLevel` (reuses portal clipping infrastructure — this is a nice synergy).
- **Main pass.** Fragment shader samples the reflection texture, distorts UVs by a scrolling normal map for waves, applies fresnel (more reflective at grazing angles), tints shallow water by depth.

Refraction is optional v1.1. Skip swimming mechanics — block the player at the waterline or let them stand on it; refine later.

### 5.4 Vegetation scatter

Two tiers:

- **Grass.** Instanced camera-facing billboards (quad with alpha-tested texture). Scatter positions generated once from a density map derived from `(slope < 0.3) × (height in grass band)`, jittered. Wind animation = sin-wave vertex displacement keyed off world position + time. Draw only within ~60 m of the camera, using one `glDrawArraysInstanced` call per chunk.
- **Trees.** Reuse the existing instanced-tree pipeline from the Basic demo, scatter positions from a second density map (lower frequency, higher slope tolerance). Swap to a billboard impostor beyond ~150 m.

Both use the same CDLOD-aware per-chunk organization so they're culled together with terrain.

## 6. Proposed file layout

```
Engine/include/world/
  Heightmap.h          Terrain.h             TerrainNode.h
  TerrainMesh.h        TerrainMaterial.h     TerrainCameraController.h
  Skybox.h             Fog.h                 SunLight.h
  ShadowMap.h          Water.h               VegetationScatter.h

Engine/src/world/      — parallel

Demo/Outdoor/
  main.cpp
  resources/heightmaps/island.png
  resources/textures/{grass,sand,dirt,rock,snow,water_normals,grass_billboard}.png
  resources/skybox/{px,nx,py,ny,pz,nz}.png
  shaders/{terrain,water,skybox,shadow,grass}.{vs,fs}
```

Heightmap/terrain code lives in a new `world/` subsystem. Keep rendering of terrain chunks going through `Object` so your existing scene-graph, culling, and shader conventions apply — `Terrain` *contains* Objects, it doesn't bypass the pipeline.

## 7. Phased delivery

I want something walkable on screen in phase 2 and keep layering from there. Each phase ends with a concrete "you can see this working" acceptance check.

**Phase 0 — Foundations.** [DONE 2026-04-21] `Heightmap` class, stb_image loader via FileSystem, unit tests for bilinear/bicubic/normal/slope sampling against a known synthetic heightmap. *Done when:* tests pass and you can load `island.png` to an in-memory float grid.

Phase 0 delivered: `Engine/include/world/Heightmap.h` + `Engine/src/world/Heightmap.cpp` wired into the `oEngine` target. Provides bilinear + Catmull-Rom bicubic sampling (with linear-extrapolated boundary taps so the bicubic reproduces linear fields exactly up to the edge), central-difference world-space gradients, surface normals, slope, world ↔ UV transform via `HeightmapTransform`, and an in-place separable Gaussian blur. Loader uses stb_image through `FileSystem::data()` so `":/heightmaps/..."` zip paths and disk fallback both work automatically. Unit tests at `Engine/tests/HeightmapTest.cpp` (19 TEST cases, 137 check-level assertions) cover construction, corner/midpoint/out-of-range sampling, normal orientation under scaling, slope, the world-space transform, and Gaussian-blur correctness (constant-field preservation, spike dampening). All 137 checks passed in the sandbox verification runner; `ctest` on the macOS build should confirm.

**Phase 1 — MVP single-chunk terrain.** [DONE 2026-04-21] One big `Object`-derived terrain, CPU-generated mesh with baked normals, simple 3-band height-only splat shader, `TerrainCameraController` with analytical ground-follow. *Done when:* you walk around a 512² heightmap in the Outdoor demo with the camera clamped to the surface.

Phase 1 delivered:

* `Engine/include/world/TerrainMeshBuilder.h` + `Engine/src/world/TerrainMeshBuilder.cpp` — bakes a Heightmap into a regular grid of 5-attribute Vertices (position/normal/uv/tangent/bitangent), CCW triangles, world-space AABB tracked during the bake. Chooses bilinear or Catmull-Rom bicubic height sampling per call; tangent/bitangent derived from the heightmap's analytic normal so the frame is orthonormal without a second pass. Returns a plain `TerrainMeshData` struct so `world/` stays independent of `utils/`; the demo wraps it into `utils::input::MeshInput` at the call site before handing it to `ObjectGenerator::mesh`, which already sets a tight local-space bounding sphere (avoiding the `project_object_cloner_bounding_sphere.md` trap).
* `Engine/include/world/TerrainCameraController.h` + `Engine/src/world/TerrainCameraController.cpp` — analytical ground-follow: exponential Y smoothing (`1 - exp(-rate·dt)`), per-frame step-height clamp, slope filter via `1 - normal.y`. Pure math helpers (`groundHeight`, `isWalkable`, `filterHorizontal`, `resolvePosition`) take plain vectors so they're unit-testable without a GL context; `updateCamera` is the one sugar method that talks to a real `Camera`.
* `Demo/Resources/shaders/terrain.vs` + `terrain.fs` — vertex shader forwards world position / normal / UV / world-Y to a fragment shader that runs a 3-band smoothstep splat (low/mid/high) over configurable band edges, shaded by a single directional "sun" + sky ambient. Portal clipping uniforms are plumbed through so terrain renders correctly inside portal sub-views without Scene knowing it's a different material.
* `Engine/tests/TerrainMeshBuilderTest.cpp` (13 TEST cases) + `Engine/tests/TerrainCameraControllerTest.cpp` (11 TEST cases) — cover invalid-input fallbacks, resolution overrides, world-position placement, AABB, UV tiling, flat vs. ramp normal orientation, index topology, triangle winding, bicubic-matches-bilinear-on-flat, orthonormal tangent frames; and controller behaviour with and without smoothing, step-height clamping, walkable-slope filtering. Sandbox verification runner reports 1076/1076 checks passing (Phase 0 suite still at 137/137).
* `Demo/Outdoor/main.cpp` + `Demo/Outdoor/CMakeLists.txt`, registered in `Demo/CMakeLists.txt`. Builds a procedural 256×256 heightmap (sum of sines with a smoothstep island falloff + a one-radius Gaussian pass) spanning 512 world units, bakes it through `TerrainMeshBuilder`, hands the result to `ObjectGenerator::mesh`, loads `terrain.vs` / `terrain.fs` via `Shader::fromFile` with the mandated `isValid()` check, loads three splat textures with white-texture fallbacks for any missing entry (only grass is reliably in the shipped zip today), spawns a plain `Camera` at the terrain-centre eye height with the far plane pushed to 1024 so the whole island is visible, and runs `TerrainCameraController::updateCamera` every frame after Scene::process. The demo deliberately uses the base `Camera` (not `CameraFPS`) and skips `Scene::setCurrentCamera`: those two together spawn a dynamic ReactPhysics3D body on the camera, whose gravity drags the viewpoint through the terrain every frame and then `Window::updateShader` overwrites whatever `TerrainCameraController` resolved. POST_BUILD copies the two shader files to `bin/` so the FileSystem disk overlay picks them up ahead of the incomplete `resources.zip`.
* `Engine/include/geometry/Scene.h` + `Engine/src/geometry/Scene.cpp` — `Scene::setPerformanceLogging(bool)` promotes the per-frame FPS + `RenderStats` report from a demo-local helper (was in `Demo/Basic/main.cpp`) to a Scene-level toggle. When enabled, `process(dt)` accumulates the frame delta and `render()` increments a frame counter; once the accumulator crosses 1 s the scene emits one `OMEGA_LOG_INFO("scene", "perf …")` line (FPS, ms/frame, draw / cull / no-bounds / considered counts, clipping on/off) and resets. Off by default; the Outdoor demo turns it on so the terrain walkthrough has a cheap, steady perf signal as later phases land.

**Phase 2 — Sky and fog.** Cubemap skybox, exponential fog in every shader. Cheap, high impact. *Done when:* the scene looks "outdoors" rather than "floating slab."

**Phase 3 — CDLOD.** Quadtree of `TerrainNode`s, shared grid mesh, vertex-shader displacement, morph, per-node AABB frustum culling, horizon skirt. *Done when:* 4 km × 4 km terrain at 60+ FPS with no visible popping.

**Phase 4 — Slope-aware texturing + triplanar + detail noise.** Upgrade the terrain shader. *Done when:* cliffs show rock with no stretching, beaches transition cleanly, screenshots stand up to scrutiny.

**Phase 5 — Water.** Flat quad, planar reflection via existing clipping plane, fresnel, scrolling normals. *Done when:* water reflects the terrain convincingly and the waterline is clean.

**Phase 6 — Sun + CSM.** Shadow pass, 3 cascades, PCF. *Done when:* hills self-shadow, objects cast shadows onto terrain, rotating the sun feels like time of day.

**Phase 7 — Vegetation.** Grass billboards + instanced trees driven by density maps. *Done when:* walking through a field feels alive and tree distribution respects slope.

**Phase 8 — Polish.** Debug overlays (wireframe, LOD color, quadtree, normals), disk caching of baked normals/splat weights, demo-scene tuning.

Phases 5, 6, 7 can reorder based on what excites you most. Phases 0–4 I'd hold the order on: each phase makes debugging the next one tractable.

## 8. Validation strategy

Per phase, three layers:

- **Unit tests.** `Heightmap` sampling, quadtree traversal invariants (children fully cover parent, depth monotonic, min/max heights consistent with samples), frustum intersection on synthetic AABBs.
- **Eyeball checks.** Dedicated debug flags for wireframe, LOD coloring, normal arrows, quadtree overlay, shadow cascade bounds. A fly-mode that ignores ground-follow, so you can inspect from above.
- **Walk test.** Run the Outdoor demo for 2 minutes along a fixed path, log frame times, assert no frame exceeded 20 ms on target hardware. Capture a screenshot at a fixed pose per phase, diff visually between phases for regressions.

Memory/file-system gotchas to keep on the checklist: every new shader needs a disk fallback and an `isValid()` check (`project_resources_zip.md`), and every chunk/Object instance must set its bounding sphere (`project_object_cloner_bounding_sphere.md`).

## 9. Open questions to pin down before Phase 3

Not blocking phases 0–2, but we'll want decisions before CDLOD:

- Target world size. 4 km² is a reasonable default; 16 km² is possible with streaming. Bigger changes the heightmap tiling question.
- Heightmap tiling for very large worlds: one giant heightmap, or a grid of heightmap tiles loaded on demand?
- Shadow cascade count: 3 is a safe default, 4 is sharper near camera, 2 is cheaper.
- Water: single global plane, or per-body water zones? Global is simpler and matches most terrain demos.
