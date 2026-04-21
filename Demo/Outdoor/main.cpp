// Outdoor demo — Phase 1 MVP walk-around. A single procedural heightmap is
// baked into a textured terrain mesh and handed through the normal Object /
// Scene pipeline so the engine can render it as just another object, while a
// `TerrainCameraController` keeps the FPS camera locked to the surface.
//
// What's actually on the screen
//   • 256×256 procedural heightmap generated at startup (sum of sines +
//     distance-based island falloff). Covers 512 world units square.
//   • 3-band splat (sand/grass/rock) driven by world Y, lit with a single
//     directional "sun" (hard-coded in the fragment shader uniforms).
//   • Camera walks at human-eye height, blocked from climbing slopes steeper
//     than the configured threshold. No physics.
//
// What this demo is deliberately NOT
//   • It's not multi-chunk. Phase 3 swaps the single bake for a CDLOD quadtree.
//   • It's not shadowed. Phase 6 adds cascaded shadow maps.
//   • There's no sky/fog/water — that's Phases 2/4/7.
// Keeping it minimal is the point: this is the "walk around and eyeball the
// heightmap" checkpoint that proves the terrain plumbing works end-to-end.

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <geometry/Object.h>
#include <geometry/ObjectTree.h>
#include <geometry/Scene.h>
#include <render/Camera.h>
#include <render/Material.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/Window.h>
#include <system/FileSystem.h>
#include <system/Log.h>
#include <system/System.h>
#include <utils/Loader.h>
#include <utils/ObjectGenerator.h>
#include <world/Heightmap.h>
#include <world/Npc.h>
#include <world/PropColliders.h>
#include <world/TerrainCameraController.h>
#include <world/TerrainMeshBuilder.h>

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::utils;
using namespace omega::world;
using namespace omega::interface;
using namespace omega::input;
using namespace omega;

namespace {

// Demo-side terrain tuning. The generator itself lives on Heightmap (engine
// side) via `Heightmap::makeProceduralIsland`; these are just the knobs this
// particular demo chooses. Splat-band boundaries in terrain.fs are tuned
// against kTerrainVerticalScale (sand up to 5, grass up to 25, rock above),
// so keep them in mind when changing the vertical scale.
constexpr int   kHeightmapResolution   = 256;
constexpr float kTerrainExtent         = 1024.0f;
constexpr float kTerrainVerticalScale  = 72.0f;

// Resolve the executable directory so we can find resources.zip whether the
// demo is launched from the repo root, from `bin/`, or from an IDE with an
// arbitrary working directory.
std::filesystem::path currentExecutableDir() {
  std::filesystem::path exePath;
#ifdef __APPLE__
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> path(size);
  _NSGetExecutablePath(path.data(), &size);
  exePath = std::filesystem::canonical(path.data()).parent_path();
#else
  exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
  return exePath;
}

}  // namespace

class OutdoorWindow : public Window {
 public:
  OutdoorWindow() : Window(1280, 720) {
    const auto exeDir = currentExecutableDir();

    // Standard `:/…` resource lookup: search for resources.zip next to the
    // executable first, then fall back to the Demo source tree. The disk
    // overlay that FileSystem sets up automatically picks up shader files
    // copied to bin/ by the POST_BUILD step.
    std::filesystem::path zipPath = exeDir / "resources.zip";
    if (!std::filesystem::exists(zipPath)) {
      zipPath = exeDir.parent_path() / "Demo" / "Resources" / "resources.zip";
      if (!std::filesystem::exists(zipPath)) {
        zipPath = exeDir.parent_path() / "Demo" / "resources.zip";
      }
    }
    fs::instance()->add(zipPath.string());

    // Build the heightmap first — the camera spawn height depends on where
    // the terrain actually is under the spawn XZ. The generator itself lives
    // in the engine (`Heightmap::makeProceduralIsland`); the demo just picks
    // the tuning constants declared above.
    heightmap_ = Heightmap::makeProceduralIsland({
      .resolution       = kHeightmapResolution,
      .horizontalExtent = kTerrainExtent,
      .verticalScale    = kTerrainVerticalScale,
      .blurRadius       = 1,
    });

    // Pre-bake the mesh. Bicubic sampling keeps chunk interiors smooth; the
    // MVP has no chunk boundaries to worry about but the option is already
    // wired for Phase 3.
    TerrainMeshParams params;
    params.resolution = 0;       // match heightmap (256×256)
    params.uvTiling = 32.0f;     // 32 tiles across 512 units → ~16 units/tile
    params.useBicubic = true;
    const auto data = TerrainMeshBuilder::build(*heightmap_, params);

    // Load the terrain shader. Following the convention in
    // `project_resources_zip.md`: always check `isValid()` because the shipped
    // zip may not contain the file and the disk overlay is the fallback.
    terrainShader_ = Shader::fromFile(4, 2,
                                      ":/shaders/terrain.vs",
                                      ":/shaders/terrain.fs");
    if (!terrainShader_ || !terrainShader_->isValid()) {
      OMEGA_LOG_ERROR("outdoor-demo",
                      "Failed to load terrain shader — demo will render an "
                      "empty window. Check that terrain.vs/terrain.fs exist "
                      "next to the executable or in resources.zip.");
      return;
    }

    // Build the terrain mesh Object first — we'll attach three Materials to
    // it below, one per splat band. The new Material::apply path binds its
    // own texture units and numeric uniforms, so we no longer hand-wire
    // `setInt` sampler assignments here.
    utils::input::MeshInput mesh;
    mesh.vertices = data.vertices;
    mesh.indices = data.indices;
    mesh.name = "Terrain";
    terrainObject_ = ObjectGenerator::mesh(std::move(mesh));
    terrainObject_->setShader(terrainShader_);

    Material waterMat = loadPBRMaterial({
      .path = ":/textures/water",
      .name = "others_0020",
      .shininess = 8.0f
    });


    // --- Band 0: dirt (low / ground) ----------------------------------------
    // Partial PBR set — the dirt pack ships BaseColor + Normal + AO but no
    // Roughness or Metallic. Those slots stay null on the Material and
    // Material::apply binds the cached neutral 1×1 fallbacks (white
    // roughness = fully matte, black metallic = dielectric) at bind time.
    Material dirtMat = loadPBRMaterial({
      .path = ":/textures/dirt",
      .name = "GroundDirtWeedsPatchy004",
      .shininess = 8.0f
    });

    // --- Band 1: grass (mid / slopes) ---------------------------------------
    // Full Poliigon PBR set (BaseColor + Normal + Roughness + AO + Metallic).
    // loadPBRMaterial probes the filesystem for each standard suffix with
    // both `_` and `-` separators and both jpg/png extensions, so mixed-
    // extension packs like this one (Normal=png, rest=jpg) load cleanly.
    Material grassMat = loadPBRMaterial({
      .path = ":/textures/grass",
      .name = "Poliigon_GrassPatchyGround_4585",
      .shininess = 8.0f
    });

    // --- Band 2: snow (high / peaks) ----------------------------------------
    // Full PBR set on disk (BaseColor + Normal + Roughness + Metallic + AO),
    // all png, with a hyphen separator (`snow-packed12-BaseColor.png` etc.).
    // The generic probe in loadPBRMaterial handles both separator
    // conventions so this Just Works alongside the underscore-style packs
    // above — no per-pack options, no call-site flags.
    Material snowMat = loadPBRMaterial({
      .path = ":/textures/snow",
      .name = "Crusted_snow2",
      .shininess = 8.0f
    });

    // Hand the three Materials to the Object in order. The terrain fragment
    // shader addresses them as `materials[0]` / `[1]` / `[2]` (dirt / grass
    // / snow). Object::render will drive Material::apply on each with
    // firstUnit = 0, 5, 10 — matching Material::kApplyUnitCount.
    terrainObject_->setMaterials({dirtMat, grassMat, snowMat});

    // Spawn the camera at the terrain centre, eye height above the surface so
    // the first frame doesn't clip through. TerrainCameraController will keep
    // it there afterwards.
    const glm::vec2 spawnXZ(0.0f, 0.0f);
    const float groundY = heightmap_->heightAtWorld(spawnXZ.x, spawnXZ.y);

    // Use the plain `Camera` (not `CameraFPS`) deliberately: `CameraFPS` walks
    // via a ReactPhysics3D dynamic rigid body and requires the scene to call
    // `setupPhysics` on it — which the Scene::setCurrentCamera path does by
    // default. With a dynamic body on the camera and no collider under the
    // terrain, gravity drags the camera through the world every frame and
    // `Window::updateShader` then overwrites whatever the terrain controller
    // resolved. Using the base `Camera` gives us WASD via plain position math
    // and leaves vertical placement entirely to the `TerrainCameraController`.
    //
    // Pitch stays at 0 at spawn so the first forward input doesn't nudge the
    // camera off the surface before the ground-follow tick catches it.
    //
    // Extend the far plane: default is 100 but the terrain is 512 across, so
    // 100 would clip the far side to the horizon. 1024 comfortably sees the
    // whole island plus headroom for Phase 2 sky/fog.
    camera_ = std::make_shared<Camera>(
        glm::vec3(spawnXZ.x, groundY + 1.7f, spawnXZ.y),
        glm::vec3(0.0f, 1.0f, 0.0f),
        /*yaw=*/-45.0f,
        /*pitch=*/0.0f);
    camera_->setPerspective(60.0f, 1280.0f, 720.0f, 0.1f, 1024.0f);
    // Base Camera's default speed (2.5 units/s) is tuned for interior scenes;
    // the island is 512 units across, so WASD at 2.5 feels like wading through
    // treacle. 15 ≈ brisk-jog pace — fast enough to cross the island in under a
    // minute, slow enough that you can still read terrain features. Bump
    // further once Phase 3 chunks out the terrain and the map grows to km-scale.
    camera_->setMovementSpeed(15.0f);
    setCamera(camera_);

    // Controller tuning: eye height 1.7 m, step-height clamp at 0.75 m per
    // frame (a little more permissive than the default 0.5 m so the camera
    // keeps up when sprinting down a slope), and 45° walkable slope which
    // matches the default `maxWalkableSlope = 0.7` (1 - cos(45°) ≈ 0.29; we
    // use 1 - normal.y which for 45° is 1 - √2/2 ≈ 0.29; 0.7 maps to ~72° so
    // the demo lets you climb fairly steep terrain). Adjust to taste.
    TerrainCameraParams cp;
    cp.eyeHeight = 1.7f;
    cp.stepHeight = 0.75f;
    cp.groundSmoothing = 10.0f;
    cp.maxWalkableSlope = 0.7f;
    // XZ radius used when pushing the camera out of tree/rock colliders.
    // 0.4 m matches a "person-wide" cylinder and keeps the camera from
    // clipping the visible trunk silhouette as you brush past a tree.
    cp.actorRadius = 0.4f;
    controller_ = TerrainCameraController(heightmap_, cp);

    // Shared obstacle set — populateForest fills this with a shrunk world-
    // space sphere per placed tree mesh and per placed rock mesh (see
    // `kTreePropRadiusScale` / `kRockPropRadiusScale` in populateForest).
    // The same set is handed to both the camera controller and each call to
    // Npc::resolveCollisions so the player and the animals collide against
    // identical geometry.
    obstacles_ = std::make_shared<PropColliderSet>();
    controller_.setObstacles(obstacles_);

    // Build the scene. We do NOT call Scene::shaders(...) because the terrain
    // uses its own shader rather than the core lit shader; Object::render
    // binds the per-object shader directly. We also do NOT call
    // `setCurrentCamera` — that call explicitly invokes `setupPhysics` on the
    // target camera, which creates a dynamic rigid body and unleashes gravity
    // on it. `current_camera_` defaults to 0, which is exactly the slot our
    // first `add(camera_)` drops into, so the render path still finds it.
    scene_ = std::make_shared<Scene>(false);
    scene_->debug(false);
    scene_->add(camera_);
    scene_->add(terrainObject_);

    // Skydome. Same six-face cubemap + skybox.vs/fs pattern the Basic demo
    // uses — ObjectGenerator::dome builds the cube mesh and attaches a
    // CubeTexture; SkyBox::render then strips translation from the view
    // matrix and swaps the depth func to GL_LEQUAL so the dome always sits
    // at the far plane behind every other scene object. No special scene
    // plumbing needed beyond add() + a prepared shader.
    skyShader_ = Shader::fromFile(4, 2,
                                  ":/shaders/skybox.vs",
                                  ":/shaders/skybox.fs");
    if (!skyShader_ || !skyShader_->isValid()) {
      OMEGA_LOG_WARN("outdoor-demo",
                     "Skybox shader failed to compile — sky will fall back to "
                     "the GL clear colour.");
    } else {
      skyShader_->setInt("skybox", 0);
      skyDome_ = ObjectGenerator::dome({
          .front  = ":/textures/skybox/front.jpg",
          .back   = ":/textures/skybox/back.jpg",
          .left   = ":/textures/skybox/left.jpg",
          .right  = ":/textures/skybox/right.jpg",
          .top    = ":/textures/skybox/top.jpg",
          .bottom = ":/textures/skybox/bottom.jpg",
      });
      skyDome_->setShader(skyShader_);
      scene_->add(skyDome_);
    }

    // Vegetation pass. Loads the core lit shader once and scatters a few
    // thousand tree/rock instances across the terrain using the heightmap
    // for terrain-aware placement (slope + band windows). Runs before
    // `scene_->prepare()` so every Object added here goes through the same
    // upload pass as the terrain and skydome — no second prepare() needed.
    // Props (trees/rocks/NPCs) use a dedicated prop.fs rather than core.fs.
    // core.fs applies the directional-light pass multiplicatively on top of
    // an `ambient * tintedColor` base (and CalcDirLight itself contains a
    // second factor of tintedColor), which effectively squares the albedo
    // and crushes mid-tones toward black — fine for the Basic demo's
    // additive point-light setup but wrong for an outdoor scene lit by a
    // single sun. prop.fs uses the plain additive formulation
    // `lit = ambient*albedo + sun*ndl*albedo`, which behaves sanely both
    // for textured meshes and for glb sub-meshes that ship without a
    // diffuse map (pushing ambient high to hide the former case was what
    // made the latter case unnaturally bright).
    //
    // The vertex stage is core.vs because it already outputs FragPos /
    // Normal / TexCoords and prop.fs uses exactly those.
    propShader_ = Shader::fromFile(4, 2,
                                   ":/shaders/core.vs",
                                   ":/shaders/prop.fs");
    if (!propShader_ || !propShader_->isValid()) {
      OMEGA_LOG_WARN("outdoor-demo",
                     "Prop shader failed to compile — trees/rocks skipped; "
                     "terrain will still render.");
    } else {
      // Baseline ambient for the shadow side of each prop. 0.35 reads as
      // "in shadow but not black"; can be tuned once sky colour becomes a
      // first-class engine concept (Phase 2).
      propShader_->setVec4("ambient", 0.35f, 0.35f, 0.35f, 1.0f);

      // Match the hard-coded sun in terrain.fs so props shade consistently
      // with the ground underneath them.
      propShader_->setVec3("sunDirection", glm::vec3(-0.5f, 0.75f, -0.4f));
      propShader_->setVec3("sunColor",     glm::vec3(0.85f, 0.80f, 0.70f));

      populateForest();
      // NPCs share the same core.vs/core.fs shader + directional sun as the
      // trees and rocks, so they must be spawned *after* the prop shader is
      // confirmed valid and the sun light has been registered.
      populateNpcs();
    }

    scene_->prepare();

    // Emit a once-per-second `scene` perf line: FPS, draw/cull counts, and
    // whether portal clipping is active. Cheap when on, compiled-out cost when
    // off. Handy for eyeballing terrain cost as Phase 3 chunking lands.
    scene_->setPerformanceLogging(true);

    OMEGA_LOG_INFO("outdoor-demo",
                   "Terrain baked: {} verts, {} indices, bounds "
                   "min=({:.1f},{:.1f},{:.1f}) max=({:.1f},{:.1f},{:.1f})",
                   data.vertices.size(), data.indices.size(),
                   data.minBound.x, data.minBound.y, data.minBound.z,
                   data.maxBound.x, data.maxBound.y, data.maxBound.z);
    printHelpBanner();
  }

  void process() override {
    if (scene_) scene_->process(m_deltaTime);
    // Ground-follow runs AFTER Scene::process so it overrides whatever Y the
    // camera's FPS handler produced. Skips cleanly if the controller has no
    // heightmap (e.g. shader load failed earlier and we bailed).
    if (camera_ && controller_.heightmap()) {
      controller_.updateCamera(*camera_, m_deltaTime);
    }
    // Advance wandering NPCs: per-NPC steering + ground-follow, then a single
    // pairwise collision-resolution pass, then commit the updated poses to
    // each NPC's mesh instances so Scene::render picks them up. The order
    // matters — `applyTransformsToMeshes` after collision means the frame's
    // draw reflects the resolved (non-overlapping) positions.
    if (!npcs_.empty()) {
      for (auto& npc : npcs_) {
        if (npc) npc->update(m_deltaTime, npcRng_);
      }
      Npc::resolveCollisions(npcs_, obstacles_.get());
      for (auto& npc : npcs_) {
        if (npc) npc->applyTransformsToMeshes();
      }
    }
    Window::process();
  }

  bool render() override {
    if (scene_) scene_->render();
    return Window::render();
  }

  void keyEvent(int state, int key, int modifier, bool repeat) override {
    const bool pressed = (state == KEY_STATE_DOWN) && !repeat;
    if (pressed) {
      switch (key) {
        case KEY_ESCAPE:
          quit();
          return;
        case KEY_H:
        case KEY_F1:
          printHelpBanner();
          return;
        case KEY_SPACE:
          // Edge-trigger the jump on key-down so holding SPACE does not
          // chain-hop (the controller also guards against that). We don't
          // fall through to Window::keyEvent — the base polls SPACE every
          // frame and would forward `JUMP` to the camera, which the base
          // Camera ignores anyway, but silencing it here keeps the intent
          // clear: jumping is a controller concern, not a camera one.
          controller_.requestJump();
          return;
        default:
          break;
      }
    }
    Window::keyEvent(state, key, modifier, repeat);
  }

 private:
  // --- Prop scattering (trees + rocks) --------------------------------------
  // Ports the Basic demo's model-cache + clone-per-instance pattern so
  // thousands of placements share a single VAO/VBO per prototype. The
  // clone step explicitly copies the local-space bounding sphere onto each
  // instance — without that, every cloned mesh registers as "no bounds" at
  // Scene::render time and frustum culling silently becomes
  // "draw everything" (see `project_object_cloner_bounding_sphere.md`).

  void applyTransformAndShader(const ObjectNodePtr& node,
                               const glm::mat4& world,
                               const std::shared_ptr<Shader>& sh) {
    if (!node) return;
    for (auto& mesh : node->meshes) {
      mesh->setModel(world);
      mesh->setShader(sh);
    }
    for (auto& child : node->children) {
      applyTransformAndShader(child, world, sh);
    }
  }

  std::shared_ptr<Object> cloneMesh(const std::shared_ptr<Object>& proto) {
    auto copy = std::make_shared<Object>(proto->getVAO(), proto->getVBO(),
                                         proto->getCount(), proto->getType());
    copy->setName(proto->name());
    copy->setTextures(proto->getTextures());
    if (auto mat = proto->getMaterial()) copy->setMaterial(*mat);
    if (const auto& sphere = proto->boundingSphere()) {
      copy->setBoundingSphere(*sphere);
    }
    return copy;
  }

  ObjectNodePtr cloneNode(const ObjectNodePtr& node) {
    if (!node) return nullptr;
    auto out = std::make_shared<ObjectNode>();
    out->mat = node->mat;
    out->meshes.reserve(node->meshes.size());
    for (const auto& m : node->meshes) out->meshes.push_back(cloneMesh(m));
    out->children.reserve(node->children.size());
    for (const auto& c : node->children) out->children.push_back(cloneNode(c));
    return out;
  }

  ObjectNodePtr cachedLoad(const std::string& path) {
    auto it = modelCache_.find(path);
    if (it != modelCache_.end()) return it->second;
    auto tree = Loader::loadModel(path);
    if (tree) modelCache_[path] = tree;
    return tree;
  }

  // `propRadiusScale` is how much of each mesh's world-space bounding sphere
  // is kept as an analytical obstacle collider. Tree bounding spheres
  // typically enclose the full canopy, so a tree wants an aggressive shrink
  // (~0.35) to approximate the trunk; rocks are closer to their visible
  // silhouette and want a gentler shrink (~0.55). Passing 0 skips the
  // collider append — handy for pure decorations.
  bool placeModel(const std::string& path, const glm::vec3& position,
                  float yawRadians, float scale,
                  float propRadiusScale) {
    auto proto = cachedLoad(path);
    if (!proto) return false;
    auto instance = cloneNode(proto);
    if (!instance) return false;
    auto world = glm::mat4(1.0f);
    world = glm::translate(world, position);
    world = glm::rotate(world, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::scale(world, glm::vec3(scale));
    applyTransformAndShader(instance, world, propShader_);
    scene_->add(instance);
    // Register analytical obstacle spheres after setModel has been applied —
    // worldBoundingSphere() reads the model matrix. Order matters: must come
    // after applyTransformAndShader above.
    if (obstacles_ && propRadiusScale > 0.0f) {
      obstacles_->addFromPlacedNode(instance, propRadiusScale);
    }
    return true;
  }

  // Scatter trees across the grass band and rocks across all non-water land.
  // Counts are chosen so a 512×512 island reads as forested without pushing
  // the draw-call budget past what frustum culling can hide off-screen.
  //
  // Placement rules:
  //   • Trees only where 3 ≤ y ≤ 22 (inside the grass splat band with a
  //     buffer at each end so the sand/snow blend zones stay clear) and
  //     where the terrain slope is gentle enough that a trunk doesn't stick
  //     out sideways.
  //   • Rocks anywhere above the water line (y ≥ 1); no slope rejection
  //     because rocks on cliffs look fine.
  // Both use a fixed RNG seed so a given terrain+count combination produces
  // the same forest run to run — makes tuning reproducible.
  void populateForest() {
    if (!heightmap_ || !propShader_ || !propShader_->isValid()) {
      OMEGA_LOG_WARN("outdoor-demo",
                     "populateForest: heightmap or prop shader missing; "
                     "skipping vegetation pass.");
      return;
    }

    const std::array<const char*, 5> trees = {
        ":/models/tree.glb",
        ":/models/tree-high.glb",
        ":/models/tree-crooked.glb",
        ":/models/tree-high-crooked.glb",
        ":/models/tree-high-round.glb",
    };
    const std::array<const char*, 3> rocks = {
        ":/models/rock-small.glb",
        ":/models/rock-wide.glb",
        ":/models/rock-large.glb",
    };

    std::mt19937 rng(1337);
    // Stay well inside the island-falloff radius (~0.75 normalized → ~0.75 *
    // 0.5 * extent = ~192 units for a 512-extent map); sampling up to 0.45
    // of the extent keeps placements on the land portion of the island.
    const float sampleHalfExtent = 0.45f * kTerrainExtent;
    std::uniform_real_distribution<float> plane(-sampleHalfExtent,
                                                 sampleHalfExtent);
    std::uniform_real_distribution<float> yaw(0.0f,
                                              glm::two_pi<float>());
    std::uniform_real_distribution<float> treeScale(0.9f, 1.8f);
    std::uniform_real_distribution<float> rockScale(0.5f, 1.6f);
    std::uniform_int_distribution<int> treeIdx(0, 4);
    std::uniform_int_distribution<int> rockIdx(0, 2);

    // Band windows (see terrain.fs: bandLowMax=5, bandMidMax=25). The tree
    // window is intentionally narrower than [5, 25] so trees don't crowd
    // the sand/snow blend seams.
    constexpr float kTreeMinY  = 3.0f;
    constexpr float kTreeMaxY  = 22.0f;
    constexpr float kRockMinY  = 1.0f;
    // Slope rejection for trees. normal.y ∈ [0,1]; 1 − normal.y ≈ sin²(θ/2)·…
    // For gentle slopes (< ~28°) this stays under 0.12; we allow up to 0.35
    // which corresponds to ~45° — steeper than that, trunks visibly lean.
    constexpr float kMaxTreeSlope = 0.35f;

    // How aggressively to shrink each prop's world-space bounding sphere
    // when registering it as an analytical obstacle. Trees' spheres enclose
    // trunk + full canopy, so 0.35 maps "visual envelope" to "trunk-ish"
    // without leaving a noticeable gap between the visible leaves and the
    // invisible wall. Rocks are closer to their own silhouette — 0.55 gives
    // a tight-but-forgiving keep-out. Tune here until the demo feels right.
    constexpr float kTreePropRadiusScale = 0.35f;
    constexpr float kRockPropRadiusScale = 0.55f;

    int treesPlaced = 0;
    constexpr int kTreeAttempts = 2500;
    for (int i = 0; i < kTreeAttempts; ++i) {
      glm::vec3 pos(plane(rng), 0.0f, plane(rng));
      const float y = heightmap_->heightAtWorld(pos.x, pos.z);
      if (y < kTreeMinY || y > kTreeMaxY) continue;
      const glm::vec3 n = heightmap_->normalAtWorld(pos.x, pos.z);
      if ((1.0f - n.y) > kMaxTreeSlope) continue;
      // Place the base of the tree exactly on the sampled ground height.
      // The glb models' local origin is at the trunk base (same as the
      // Basic demo places them at pos.y = 0 on its y = 0 ground plane), so
      // this puts the trunk standing on the surface. We deliberately do NOT
      // subtract a small epsilon here: the terrain mesh is bicubic-sampled
      // while heightAtWorld is bilinear, so the rendered ground can sit a
      // few cm above the sampled height in places. Sinking trees below the
      // bilinear sample risks burying the base under the bicubic mesh.
      pos.y = y;
      if (placeModel(trees[treeIdx(rng)], pos, yaw(rng), treeScale(rng),
                     kTreePropRadiusScale)) {
        ++treesPlaced;
      }
    }

    int rocksPlaced = 0;
    constexpr int kRockAttempts = 900;
    for (int i = 0; i < kRockAttempts; ++i) {
      glm::vec3 pos(plane(rng), 0.0f, plane(rng));
      const float y = heightmap_->heightAtWorld(pos.x, pos.z);
      if (y < kRockMinY) continue;
      pos.y = y;  // same rationale as the tree case — don't sink below mesh
      if (placeModel(rocks[rockIdx(rng)], pos, yaw(rng), rockScale(rng),
                     kRockPropRadiusScale)) {
        ++rocksPlaced;
      }
    }

    OMEGA_LOG_INFO("outdoor-demo",
                   "Populated terrain with {} trees and {} rocks "
                   "(attempts: {} / {}; {} obstacle spheres registered)",
                   treesPlaced, rocksPlaced, kTreeAttempts, kRockAttempts,
                   obstacles_ ? obstacles_->size() : 0);
  }

  // Spawn a pack of mixed animal NPCs on the grass band. Each species picks
  // up NpcParams tuned for its scale and gait (an elephant plods at 1.2
  // units/s, a fox trots at 2.5, a chick scurries at 1.8 with a small
  // footprint), and the models are loaded via the pre-transformed variant of
  // the Loader so multi-node GLBs (Kenney's animals are each split into
  // body/legs/tail/head) render as a single coherent animal.
  //
  // The spawn seed is fixed so the roster is reproducible from run to run —
  // makes it easier to judge tuning changes. Species and positions can still
  // vary across the world because the wander loop re-randomizes targets.
  void populateNpcs() {
    if (!heightmap_ || !propShader_ || !propShader_->isValid()) {
      OMEGA_LOG_WARN("outdoor-demo",
                     "populateNpcs: heightmap or prop shader missing; "
                     "animals skipped.");
      return;
    }

    // Per-species tuning. Scale is in world units; Kenney's animals are
    // roughly 1 unit = "tile" size so a scale of ~1 gives a plausible
    // real-world footprint against the 1.7 m camera eye height used above.
    // `fence` shares the same half-extent the trees use so animals stay on
    // the land portion of the island.
    struct SpeciesDef {
      const char* modelPath;
      float       scale;
      float       moveSpeed;
      float       wanderRadius;
      float       collisionRadius;  // 0 → auto from model bounds
      float       bobFrequency;
    };
    const std::array<SpeciesDef, 12> species = {{
        {":/models/animals/animal-fox.glb",      1.0f, 2.2f, 20.f, 0.0f, 12.f},
        {":/models/animals/animal-deer.glb",     1.1f, 2.0f, 22.f, 0.0f, 10.f},
        {":/models/animals/animal-bunny.glb",    0.6f, 2.4f, 10.f, 0.0f, 14.f},
        {":/models/animals/animal-cow.glb",      1.1f, 1.1f, 12.f, 0.0f,  8.f},
        {":/models/animals/animal-dog.glb",      0.9f, 2.3f, 18.f, 0.0f, 12.f},
        {":/models/animals/animal-pig.glb",      0.9f, 1.4f, 10.f, 0.0f, 10.f},
        {":/models/animals/animal-cat.glb",      0.65f,2.1f, 14.f, 0.0f, 13.f},
        {":/models/animals/animal-elephant.glb", 1.6f, 1.1f, 22.f, 1.6f,  6.f},
        {":/models/animals/animal-giraffe.glb",  1.3f, 1.3f, 22.f, 1.1f,  7.f},
        {":/models/animals/animal-hog.glb",      1.0f, 1.7f, 16.f, 0.0f, 10.f},
        {":/models/animals/animal-panda.glb",    1.0f, 1.3f, 12.f, 0.0f,  8.f},
        {":/models/animals/animal-chick.glb",    0.4f, 1.8f,  6.f, 0.0f, 18.f},
    }};

    // Kenney mini-character pack (character-a .. character-r). All 18 variants
    // share the same base rig / scale, so the tuning is uniform — only the
    // `modelPath` changes. Walk speed sits between the slow pig and the
    // brisk fox: ~1.6 m/s reads as a relaxed stroll against the 1.7 m camera.
    // Bob frequency is lower than the animals' because a human stride is
    // ~2 Hz; we run the sin at double that so both feet "hit the ground"
    // once per cycle. Collision radius is left on auto — the character model
    // is a boxy humanoid and the computed footprint matches the visual base.
    const std::array<SpeciesDef, 18> characters = {{
        {":/models/people/character-a.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-b.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-c.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-d.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-e.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-f.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-g.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-h.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-i.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-j.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-k.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-l.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-m.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-n.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-o.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-p.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-q.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
        {":/models/people/character-r.glb", 1.0f, 1.6f, 14.f, 0.0f, 4.0f},
    }};

    // Shared fencing + placement tuning. Same grass-band window the forest
    // uses — the NPC spawn loop rejects picks outside the band so animals
    // don't end up hip-deep in sand or perched on snow peaks.
    constexpr float kAnimalMinY = 2.5f;
    constexpr float kAnimalMaxY = 20.0f;
    constexpr float kSampleHalfExtent = 0.40f * kTerrainExtent;

    std::mt19937 rng(4242);
    std::uniform_real_distribution<float> plane(-kSampleHalfExtent,
                                                 kSampleHalfExtent);

    // How many copies of each species we attempt to spawn. Two of each plus
    // a random roll across all species keeps the camera from ever standing
    // in a zone with no wildlife visible on the 512-wide island.
    constexpr int kCopiesPerSpecies = 2;
    constexpr int kRandomExtra = 12;
    std::uniform_int_distribution<int> speciesIdx(
        0, static_cast<int>(species.size()) - 1);

    auto attemptSpawn = [&](const SpeciesDef& def) -> bool {
      // Parse once, cache once. Loaders are cheap to re-run but cloning the
      // shared tree per-instance keeps the GL handles shared.
      auto it = animalCache_.find(def.modelPath);
      ObjectNodePtr proto;
      if (it == animalCache_.end()) {
        proto = Loader::loadModelPreTransformed(def.modelPath);
        if (!proto) {
          OMEGA_LOG_WARN("outdoor-demo",
                         "Failed to load animal model {}", def.modelPath);
          return false;
        }
        animalCache_[def.modelPath] = proto;
      } else {
        proto = it->second;
      }

      // Try a handful of spawn points; accept the first one that lands on
      // the grass band with gentle enough slope. Give up silently if nothing
      // sticks — dropping a lone animal shouldn't block the rest of the
      // roster.
      for (int attempt = 0; attempt < 30; ++attempt) {
        const glm::vec2 xz(plane(rng), plane(rng));
        const float y = heightmap_->heightAtWorld(xz.x, xz.y);
        if (y < kAnimalMinY || y > kAnimalMaxY) continue;
        const glm::vec3 n = heightmap_->normalAtWorld(xz.x, xz.y);
        if ((1.0f - n.y) > 0.30f) continue;

        NpcParams p;
        p.scale             = def.scale;
        p.moveSpeed         = def.moveSpeed;
        p.wanderRadius      = def.wanderRadius;
        p.collisionRadius   = def.collisionRadius;
        p.bobFrequency      = def.bobFrequency;
        p.maxWalkableSlope  = 0.45f;
        p.fenceCenter       = glm::vec3(0.0f);
        p.halfExtentXZ      = glm::vec2(kSampleHalfExtent);
        p.groundOffset      = 0.0f;

        auto npc = Npc::spawn(proto, propShader_, heightmap_, xz, p,
                              def.modelPath);
        if (!npc) return false;

        // Hand the NPC's cloned tree to the scene so the renderer traverses
        // it each frame. `applyTransformsToMeshes` was already called once
        // in spawn() so the first frame doesn't flash at the origin.
        scene_->add(npc->root());
        npcs_.push_back(std::move(npc));
        return true;
      }
      return false;
    };

    int animalsSpawned = 0;
    for (int copy = 0; copy < kCopiesPerSpecies; ++copy) {
      for (const auto& def : species) {
        if (attemptSpawn(def)) ++animalsSpawned;
      }
    }
    for (int i = 0; i < kRandomExtra; ++i) {
      const auto& def = species[speciesIdx(rng)];
      if (attemptSpawn(def)) ++animalsSpawned;
    }

    // Characters: one of each variant, plus a few random extras so the
    // island feels populated. The same `attemptSpawn` path handles them —
    // tuning differences live on the SpeciesDef, and the animalCache_ map
    // keys on the full resource path so "animal" and "character" entries
    // coexist without collision.
    constexpr int kCharacterExtras = 6;
    std::uniform_int_distribution<int> characterIdx(
        0, static_cast<int>(characters.size()) - 1);

    int charactersSpawned = 0;
    for (const auto& def : characters) {
      if (attemptSpawn(def)) ++charactersSpawned;
    }
    for (int i = 0; i < kCharacterExtras; ++i) {
      const auto& def = characters[characterIdx(rng)];
      if (attemptSpawn(def)) ++charactersSpawned;
    }

    OMEGA_LOG_INFO("outdoor-demo",
                   "Spawned {} animal NPCs across {} species and {} "
                   "character NPCs across {} variants",
                   animalsSpawned, species.size(),
                   charactersSpawned, characters.size());
  }

  void printHelpBanner() const {
    std::cout
        << "\n=== Outdoor Demo (Phase 1 MVP) ================================\n"
        << "Procedural 256x256 heightmap, 512x512 world units, splat-lit.\n"
        << "\n"
        << "Movement:   WASD / mouse look  |  SPACE jump\n"
        << "System:     ESC quit  |  H / F1 help\n"
        << "\n"
        << "Camera is locked to terrain height (eye=1.7m), slopes steeper\n"
        << "than ~72 degrees block horizontal motion. Jumps are ballistic\n"
        << "(no double-jump), ground-follow resumes on landing.\n"
        << "\n"
        << "Wandering animal NPCs (fox, deer, bunny, cow, dog, pig, cat,\n"
        << "elephant, giraffe, hog, panda, chick) plus a cast of human\n"
        << "character variants roam the grass band with sphere-sphere\n"
        << "collision avoidance and procedural walk-bob.\n"
        << "==============================================================\n"
        << std::endl;
  }

  std::shared_ptr<Scene> scene_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<Heightmap> heightmap_;
  // Three splat-band Materials live on the Object itself (see setMaterials
  // above) — their internal shared_ptr<Texture>s keep the GL ids alive for
  // the lifetime of the window, so we no longer need per-texture member
  // fields on OutdoorWindow.
  std::shared_ptr<Object> terrainObject_;
  std::shared_ptr<Shader> terrainShader_;
  std::shared_ptr<Object> skyDome_;
  std::shared_ptr<Shader> skyShader_;
  // Shared shader for every tree/rock instance placed by populateForest().
  // The core lit shader already handles per-object Materials + lights, which
  // is exactly what the glb props ship with, so there's no per-prop shader
  // wiring beyond this one handle.
  std::shared_ptr<Shader> propShader_;
  // One parsed glb per unique path; cloneNode() produces lightweight
  // instances that share the prototype's VAO/VBO so scattering thousands of
  // props costs one asset parse per distinct model.
  std::unordered_map<std::string, ObjectNodePtr> modelCache_;
  TerrainCameraController controller_;

  // Analytical obstacle spheres for player/NPC prop collision. Shared with
  // the TerrainCameraController (which pushes the camera out each frame) and
  // with Npc::resolveCollisions (which pushes animals out). Populated inside
  // placeModel() as each tree/rock is added to the scene.
  std::shared_ptr<PropColliderSet> obstacles_;

  // NPC roster and per-species prototype cache. Prototypes live in their own
  // map (not `modelCache_`) because they're loaded via
  // `Loader::loadModelPreTransformed` — different flags than the tree/rock
  // loads — and we never want a cached fox prototype to be handed out as a
  // rock instance or vice versa.
  std::vector<std::shared_ptr<Npc>> npcs_;
  std::unordered_map<std::string, ObjectNodePtr> animalCache_;
  // Shared RNG for NPC steering decisions. Seeded once so a given roster
  // produces a reproducible initial wander plan; per-frame advances of the
  // RNG state are fine because all NPC updates happen on the main thread.
  std::mt19937 npcRng_{0xCAFEF00Du};
};

int main(int /*argc*/, char** /*argv*/) {
  OSystem::init();

  auto window = std::make_shared<OutdoorWindow>();
  Window::setInstance(window);

  while (window->isRuning()) {
    window->process();
    window->clear();
    window->render();
    window->swap();
  }

  return 0;
}
