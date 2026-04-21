// Portal demo — POC / test harness for every portal feature implemented
// across Phase 0 and Phase 1 of the completion plan.
//
// By default the demo loads `showcase_scene.json` — a single-room showroom
// with one portal of every archetype (mirror, paired doorways, closed door,
// non-passable window). A live hotkey panel lets you toggle the stencil vs
// FBO pipeline, change the recursion depth, flip overlay settings, open and
// close the demo door, and dump per-frame culling stats. Works as both a
// smoke test (press keys, see things change) and as the reference for what
// the engine can currently do.

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <geometry/Object.h>
#include <geometry/Point3.h>
#include <geometry/Portal.h>
#include <geometry/PortalRenderer.h>
#include <geometry/Scene.h>
#include <render/CameraFPS.h>
#include <render/Material.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/Window.h>
#include <system/FileSystem.h>
#include <system/Log.h>
#include <system/System.h>
#include <utils/PortalSceneLoader.h>

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::utils;
using namespace omega::interface;
using namespace omega::input;
using namespace omega;

namespace {

// Mirror tint presets cycled by the `T` hotkey. The first entry is white so
// that cycling lets you A/B a tinted mirror against a neutral reference
// without touching the intensity slider.
constexpr struct MirrorTint {
  const char *name;
  float r, g, b;
} kMirrorTints[] = {
    {"white (neutral)", 1.00f, 1.00f, 1.00f},
    {"cool blue",       0.55f, 0.75f, 1.00f},
    {"warm amber",      1.00f, 0.80f, 0.45f},
    {"toxic green",     0.55f, 1.00f, 0.65f},
    {"rose",            1.00f, 0.65f, 0.75f},
};
constexpr int kMirrorTintCount =
    sizeof(kMirrorTints) / sizeof(kMirrorTints[0]);

void printHelpBanner(bool useStencilPortals, int recursionDepth) {
  std::cout << "\n=== Portal POC Showcase =====================================\n"
            << "Single room with every portal variant the engine supports:\n"
            << "  * Mirror (north wall)   - self-linked, tinted overlay\n"
            << "  * Doorways  (E <-> W)   - paired, oblique clipping,\n"
            << "                            nested recursion when facing both\n"
            << "  * Window  (south-left)  - self-linked, passable=false\n"
            << "  * Closed door (south-right) - open=false, toggle with O\n"
            << "\n"
            << "Movement:   WASD / mouse look  |  ESC quit\n"
            << "Pipeline:   P   toggle stencil (Phase 1.4 B) <-> FBO (A)\n"
            << "Depth:      1/2/3/4  set max recursion depth (current: "
            << recursionDepth << ")\n"
            << "Mirror:     M   toggle overlay on/off\n"
            << "            T   cycle mirror tint preset\n"
            << "Door:       O   toggle south-right door open/closed\n"
            << "Portals:    R   toggle portal rendering entirely\n"
            << "Stats:      L   log last-frame cull/draw counters\n"
            << "Help:       H or F1   reprint this banner\n"
            << "\n"
            << "Starting pipeline: "
            << (useStencilPortals ? "stencil (Phase 1.4 option B)"
                                  : "FBO (Phase 1.4 option A)")
            << "\n"
            << "================================================================\n"
            << std::endl;
}

// Search order for the scene JSON, executable-relative. First hit wins.
// The showcase scene is preferred; the older tunnel/portal scenes are kept
// as fallbacks so this binary still runs against older working trees.
std::filesystem::path findSceneFile(const std::filesystem::path &exeDir,
                                    const std::string &explicitPath) {
  if (!explicitPath.empty()) {
    return std::filesystem::path(explicitPath);
  }
  const char *candidates[] = {
      "showcase_scene.json",
      "tunnel_scene.json",
      "portal_scene.json",
  };
  // First look alongside the executable (POST_BUILD copies the JSON here).
  for (const char *name : candidates) {
    auto p = exeDir / name;
    if (std::filesystem::exists(p)) return p;
  }
  // Then look in the source tree (Demo/Portal/...) — useful when running
  // the binary from an IDE with cwd elsewhere.
  const auto sourceDir = exeDir.parent_path() / "Demo" / "Portal";
  for (const char *name : candidates) {
    auto p = sourceDir / name;
    if (std::filesystem::exists(p)) return p;
  }
  return {};
}

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

class PortalWindow : public Window {
 public:
  // `useStencilPortals`   — start with stencil-based portal rendering on.
  // `initialDepth`        — max recursion depth baked in at startup; can be
  //                         changed at runtime via the 1/2/3/4 hotkeys.
  // `explicitScenePath`   — optional `--scene <path>` override.
  PortalWindow(bool useStencilPortals, int initialDepth,
               const std::string &explicitScenePath)
      : Window(1280, 720),
        useStencilPortals_(useStencilPortals),
        recursionDepth_(initialDepth) {
    const auto exeDir = currentExecutableDir();

    // Hook up the filesystem layer that backs `:/...` lookups in the loader
    // and shaders. We look in the executable directory first, then fall
    // back to the source tree.
    std::filesystem::path zipPath = exeDir / "resources.zip";
    if (!std::filesystem::exists(zipPath)) {
      zipPath = exeDir.parent_path() / "Demo" / "Resources" / "resources.zip";
      if (!std::filesystem::exists(zipPath)) {
        zipPath = exeDir.parent_path() / "Demo" / "resources.zip";
      }
    }
    fs::instance()->add(zipPath.string());

    const auto scenePath = findSceneFile(exeDir, explicitScenePath);
    if (scenePath.empty()) {
      OMEGA_LOG_WARN("portal-demo",
                     "No scene JSON found (looked for showcase_scene.json, "
                     "tunnel_scene.json, portal_scene.json). Demo will "
                     "display an empty window.");
      return;
    }
    OMEGA_LOG_INFO("portal-demo", "Loading scene: {}", scenePath.string());

    auto loader = std::make_shared<PortalSceneLoader>();
    scene_ = loader->loadFromFile(scenePath.string());
    if (!scene_) {
      OMEGA_LOG_WARN("portal-demo", "Scene load failed — empty window will "
                                    "render but nothing else.");
      return;
    }

    // Stash named portals so hotkeys can tweak them live. The loader gives
    // us a map keyed by JSON `id`.
    portalsById_ = loader->getPortals();

    const auto cameraConfig = loader->getCameraConfig();
    glm::vec3 cameraPos = cameraConfig.position;
    // Clamp the camera floor-ward to keep it out of physics edge cases.
    cameraPos.y = std::max(cameraPos.y, 1.0f);
    camera_ = std::make_shared<CameraFPS>(cameraPos,
                                          glm::vec3(0.0f, 1.0f, 0.0f),
                                          cameraConfig.yaw,
                                          cameraConfig.pitch);
    setCamera(camera_);
    camera_->setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

    if (auto portalRenderer = scene_->getPortalRenderer()) {
      portalRenderer->setMaxRecursionDepth(recursionDepth_);
      portalRenderer->setStencilMode(useStencilPortals_);
      portalRenderer->setEnabled(true);
    }

    scene_->add(camera_);
    scene_->setCurrentCamera(0);
  }

  void process() override {
    if (scene_) scene_->process(m_deltaTime);
    Window::process();
  }

  bool render() override {
    if (scene_) scene_->render();
    return Window::render();
  }

  // One place to resolve every hotkey, centralising the "look up a named
  // portal" pattern and the logging that announces each state change. Only
  // fires on the DOWN edge so holding a key doesn't spam toggles.
  void keyEvent(int state, int key, int modifier, bool repeat) override {
    const bool pressed = (state == KEY_STATE_DOWN) && !repeat;

    if (pressed) {
      switch (key) {
        case KEY_ESCAPE:
          quit();
          return;
        case KEY_H:
        case KEY_F1:
          printHelpBanner(isStencilActive(), recursionDepth_);
          return;
        case KEY_P:
          togglePipeline();
          return;
        case KEY_1:
          setRecursionDepth(1);
          return;
        case KEY_2:
          setRecursionDepth(2);
          return;
        case KEY_3:
          setRecursionDepth(3);
          return;
        case KEY_4:
          setRecursionDepth(4);
          return;
        case KEY_O:
          toggleClosedDoor();
          return;
        case KEY_M:
          toggleMirrorOverlay();
          return;
        case KEY_T:
          cycleMirrorTint();
          return;
        case KEY_R:
          togglePortalRendererEnabled();
          return;
        case KEY_L:
          logRenderStats();
          return;
        default:
          break;
      }
    }
    // Everything we don't intercept falls through to the base Window so WASD
    // + mouse look still work.
    Window::keyEvent(state, key, modifier, repeat);
  }

 private:
  // ---- Hotkey handlers ---------------------------------------------------

  void togglePipeline() {
    auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
    if (!pr) return;
    const bool now = !pr->isStencilMode();
    pr->setStencilMode(now);
    useStencilPortals_ = now;
    OMEGA_LOG_INFO("portal-demo", "Portal pipeline -> {}",
                   now ? "stencil (1.4 B)" : "FBO (1.4 A)");
  }

  void setRecursionDepth(int depth) {
    auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
    if (!pr) return;
    recursionDepth_ = depth;
    pr->setMaxRecursionDepth(depth);
    OMEGA_LOG_INFO("portal-demo", "Max portal recursion depth -> {}", depth);
  }

  void toggleClosedDoor() {
    auto door = portal("closed_door_south");
    if (!door) {
      OMEGA_LOG_WARN("portal-demo",
                     "No portal named 'closed_door_south' in this scene");
      return;
    }
    const bool now = !door->isOpen();
    door->setOpen(now);
    OMEGA_LOG_INFO("portal-demo", "closed_door_south.open = {}", now);
  }

  void toggleMirrorOverlay() {
    auto mirror = portal("mirror_north");
    if (!mirror) {
      OMEGA_LOG_WARN("portal-demo",
                     "No portal named 'mirror_north' in this scene");
      return;
    }
    const bool now = !mirror->hasMirrorOverlay();
    // Preserve the current intensity / tint when flipping the flag.
    mirror->setMirrorOverlay(now, mirror->getMirrorIntensity(),
                             mirror->getMirrorTint());
    OMEGA_LOG_INFO("portal-demo", "mirror_north.overlay = {}", now);
  }

  void cycleMirrorTint() {
    auto mirror = portal("mirror_north");
    if (!mirror) {
      OMEGA_LOG_WARN("portal-demo",
                     "No portal named 'mirror_north' in this scene");
      return;
    }
    mirrorTintIndex_ = (mirrorTintIndex_ + 1) % kMirrorTintCount;
    const auto &t = kMirrorTints[mirrorTintIndex_];
    mirror->setMirrorOverlay(true, mirror->getMirrorIntensity(),
                             glm::vec3(t.r, t.g, t.b));
    OMEGA_LOG_INFO("portal-demo", "mirror_north.tint -> {} ({:.2f},{:.2f},{:.2f})",
                   t.name, t.r, t.g, t.b);
  }

  void togglePortalRendererEnabled() {
    auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
    if (!pr) return;
    const bool now = !pr->isEnabled();
    pr->setEnabled(now);
    OMEGA_LOG_INFO("portal-demo", "PortalRenderer enabled = {}", now);
  }

  void logRenderStats() {
    if (!scene_) return;
    const auto &s = scene_->lastRenderStats();
    OMEGA_LOG_INFO(
        "portal-demo",
        "stats: considered={} drawn={} culledFrustum={} drawnNoBounds={} "
        "clippingActive={}",
        s.considered, s.drawn, s.culledFrustum, s.drawnNoBounds,
        s.clippingActive ? "yes" : "no");
  }

  // ---- Helpers -----------------------------------------------------------

  std::shared_ptr<Portal> portal(const std::string &id) const {
    auto it = portalsById_.find(id);
    if (it == portalsById_.end()) return nullptr;
    return it->second;
  }

  bool isStencilActive() const {
    auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
    return pr ? pr->isStencilMode() : useStencilPortals_;
  }

  std::shared_ptr<Scene> scene_;
  std::shared_ptr<CameraFPS> camera_;
  std::map<std::string, std::shared_ptr<Portal>> portalsById_;

  bool useStencilPortals_{false};
  int recursionDepth_{2};
  int mirrorTintIndex_{0};
};

int main(int argc, char *argv[]) {
  OSystem::init();

  // Minimal arg parsing. Runtime hotkeys cover most toggles; we only expose
  // the ones that need to be set *before* the GL context exists or before
  // the scene loads.
  bool useStencilPortals = false;
  int initialDepth = 2;
  std::string explicitScenePath;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--stencil-portals" || a == "-s") {
      useStencilPortals = true;
    } else if (a == "--fbo-portals") {
      useStencilPortals = false;
    } else if (a == "--scene" && i + 1 < argc) {
      explicitScenePath = argv[++i];
    } else if (a == "--depth" && i + 1 < argc) {
      initialDepth = std::max(1, std::atoi(argv[++i]));
    } else if (a == "--help" || a == "-h") {
      std::cout << "Usage: Portal [--stencil-portals|-s] [--fbo-portals] "
                   "[--scene <path>] [--depth N]\n"
                   "Runtime hotkeys are printed when the demo starts; press "
                   "H or F1 in-game to re-print them.\n";
      return 0;
    }
  }

  auto window = std::make_shared<PortalWindow>(useStencilPortals,
                                               initialDepth,
                                               explicitScenePath);
  Window::setInstance(window);

  printHelpBanner(useStencilPortals, initialDepth);

  while (window->isRuning()) {
    window->process();
    window->clear();
    window->render();
    window->swap();
  }

  return 0;
}
