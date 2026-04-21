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

#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <geometry/Object.h>
#include <geometry/Scene.h>
#include <render/Camera.h>
#include <render/Material.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/Window.h>
#include <system/FileSystem.h>
#include <system/Log.h>
#include <system/System.h>
#include <utils/ObjectGenerator.h>
#include <world/Heightmap.h>
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

// Heightmap grid size. 256×256 is dense enough to show smooth hills on a
// single screen without making the bake sluggish, and matches the CDLOD plan's
// "chunk tile" sample size so Phase 3 can reuse this shape directly.
constexpr int kHeightmapResolution = 256;

// World span of the terrain. 512 units at ~1 unit per metre puts the whole
// mesh inside a comfortable walking radius without pushing outside the current
// default far-plane of 100 — which is why we extend the far plane below.
constexpr float kTerrainExtent = 512.0f;

// Max world height above sea level. Tuned so the three splat bands (sand up to
// 5, grass up to 25, rock above) all get screen time at default camera height.
constexpr float kTerrainVerticalScale = 40.0f;

// Island falloff: samples near the border are pulled down so the demo feels
// like a finite island rather than a plane that just happens to end. Keeping
// the falloff analytic (smoothstep on normalized radius) avoids another
// texture dependency.
float islandFalloff(float u, float v) {
  const float dx = u - 0.5f;
  const float dz = v - 0.5f;
  const float r = std::sqrt(dx * dx + dz * dz) * 2.0f;  // 0 at centre, 1 at edge
  // Start pulling down at r=0.75, reach zero by r=1. The inner 75% of the
  // radius is unaffected, so the camera spawn area in the middle stays hilly.
  return 1.0f - glm::smoothstep(0.75f, 1.0f, r);
}

// Sum of a handful of sines at increasing frequencies — cheap proc-gen that
// produces something that reads as natural rolling terrain without shipping an
// asset. Values normalized to [0,1]; the HeightmapTransform handles scaling
// them to world space.
float heightAt(float u, float v) {
  const float twoPi = glm::two_pi<float>();

  const float low =
      0.5f + 0.5f * std::sin((u * 1.7f + v * 2.1f) * twoPi);
  const float mid =
      0.5f + 0.5f * std::sin((u * 3.3f - v * 2.6f) * twoPi + 1.2f);
  const float high =
      0.5f + 0.5f * std::sin((u * 6.1f + v * 5.4f) * twoPi + 2.8f);
  const float detail =
      0.5f + 0.5f * std::sin((u * 13.0f + v * 11.0f) * twoPi + 0.4f);

  // Weighted sum; the detail octave barely contributes but stops the surface
  // from looking too smooth at grazing angles.
  float h = 0.55f * low + 0.28f * mid + 0.12f * high + 0.05f * detail;
  h *= islandFalloff(u, v);

  // Clamp to [0,1]: the falloff can push results slightly negative from noise
  // and we want the raw sample to match Heightmap's [0,1] contract.
  if (h < 0.0f) h = 0.0f;
  if (h > 1.0f) h = 1.0f;
  return h;
}

// Procedurally build a heightmap in memory. Same shape as one loaded from a
// PNG — the rest of the engine doesn't care how samples got there.
std::shared_ptr<Heightmap> makeProceduralHeightmap() {
  std::vector<float> samples(kHeightmapResolution * kHeightmapResolution);
  for (int z = 0; z < kHeightmapResolution; ++z) {
    const float v = float(z) / float(kHeightmapResolution - 1);
    for (int x = 0; x < kHeightmapResolution; ++x) {
      const float u = float(x) / float(kHeightmapResolution - 1);
      samples[z * kHeightmapResolution + x] = heightAt(u, v);
    }
  }

  HeightmapTransform transform;
  transform.origin = glm::vec2(-0.5f * kTerrainExtent, -0.5f * kTerrainExtent);
  transform.horizontalScale = kTerrainExtent;
  transform.verticalScale = kTerrainVerticalScale;
  transform.verticalOffset = 0.0f;

  auto heightmap = std::make_shared<Heightmap>(std::move(samples),
                                                kHeightmapResolution,
                                                kHeightmapResolution,
                                                transform);
  // A single-radius blur softens the highest-frequency sine so the surface
  // reads as "gentle hills" rather than "noise". Cheap; done once at load.
  heightmap->gaussianBlur(1);
  return heightmap;
}

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

// Try to load a texture by resource path; fall back to a 1×1 white texture if
// the lookup fails. Resources.zip may not ship every intended "sand/rock" art
// asset yet — white fallbacks let the demo keep rendering while the splat
// shader still paints bands via its tint / lighting math.
std::shared_ptr<Texture> loadOrWhite(const std::string& path,
                                     const std::string& name) {
  auto tex = std::make_shared<Texture>();
  if (tex->load(path, name)) return tex;
  OMEGA_LOG_WARN("outdoor-demo",
                 "Texture '{}' missing — using white fallback for '{}'",
                 path, name);
  auto white = Texture::createWhiteTexture();
  white->name(name);
  return white;
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
    // the terrain actually is under the spawn XZ.
    heightmap_ = makeProceduralHeightmap();

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

    // Bind the three splat samplers up-front so the fragment shader picks the
    // right texture slots when Object::render activates textures in order.
    terrainShader_->setInt("texLow", 0);
    terrainShader_->setInt("texMid", 1);
    terrainShader_->setInt("texHigh", 2);

    // Load the three splat textures. Only grass is guaranteed to be in the
    // shipped resources.zip today; sand/rock fall back to white if absent so
    // the demo still runs against an older zip. The shader already tints via
    // lighting so a plain white fallback reads as lit snow-ish/sand-ish bands.
    texLow_ = loadOrWhite(":/textures/concreteTexture.png", "terrainLow");
    texMid_ = loadOrWhite(":/textures/pbr/grass/albedo.png", "terrainMid");
    texHigh_ = loadOrWhite(":/textures/bricks2.jpg", "terrainHigh");

    // Wrap the baked CPU data into the generator's input struct. We don't
    // supply `input.textures` by name here because the splat shader keys on
    // explicit sampler uniforms, not on texture-name matching; we add them
    // afterwards so the render-time texture binding order is well-defined.
    utils::input::MeshInput mesh;
    mesh.vertices = data.vertices;
    mesh.indices = data.indices;
    mesh.name = "Terrain";
    terrainObject_ = ObjectGenerator::mesh(std::move(mesh));
    terrainObject_->setShader(terrainShader_);
    terrainObject_->addTexture(texLow_);
    terrainObject_->addTexture(texMid_);
    terrainObject_->addTexture(texHigh_);

    // Set a mild per-object material so any future lit pipeline (e.g. Phase 6)
    // has sensible defaults. The Phase 1 shader ignores this.
    terrainObject_->setMaterial(Material{.shininess = 8.0f});

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
    controller_ = TerrainCameraController(heightmap_, cp);

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
        << "==============================================================\n"
        << std::endl;
  }

  std::shared_ptr<Scene> scene_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<Heightmap> heightmap_;
  std::shared_ptr<Object> terrainObject_;
  std::shared_ptr<Shader> terrainShader_;
  std::shared_ptr<Texture> texLow_;
  std::shared_ptr<Texture> texMid_;
  std::shared_ptr<Texture> texHigh_;
  TerrainCameraController controller_;
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
