#include "CastleWindow.h"

#include <algorithm>
#include <filesystem>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <geometry/PortalRenderer.h>
#include <system/FileSystem.h>
#include <system/Log.h>
#include <utils/PortalSceneLoader.h>

using omega::geometry::Portal;
using omega::geometry::Scene;
using omega::render::CameraFPS;
using omega::render::Window;
using omega::utils::PortalSceneLoader;

namespace omega::demo::castle {

namespace {

// Executable directory resolution. The Portal demo ships a similar helper;
// the Castle demo re-implements it locally rather than linking against it
// so the two demos stay independent and editable without cross-demo
// refactors.
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

std::filesystem::path findSceneFile(const std::filesystem::path &exeDir,
                                    const std::string &explicitPath) {
  if (!explicitPath.empty()) {
    return std::filesystem::path(explicitPath);
  }
  const char *candidates[] = {
      "castle_labyrinth.json",
      // Useful during iteration — lets the same binary run a stub scene
      // if the big labyrinth file is still being authored.
      "castle_scene.json",
  };
  for (const char *name : candidates) {
    auto p = exeDir / name;
    if (std::filesystem::exists(p)) return p;
  }
  const auto sourceDir = exeDir.parent_path() / "Demo" / "Castle";
  for (const char *name : candidates) {
    auto p = sourceDir / name;
    if (std::filesystem::exists(p)) return p;
  }
  return {};
}

void printHelpBanner(bool useStencilPortals, int recursionDepth) {
  OMEGA_LOG_INFO("castle-demo",
                 "=== Castle Labyrinth POC ==============================\n"
                 "Movement:   WASD / mouse look  |  ESC quit\n"
                 "Pipeline:   P   toggle stencil <-> FBO\n"
                 "Depth:      1/2/3/4  max portal recursion (cur: {})\n"
                 "Doors:      O   cycle & toggle a closed sliding door\n"
                 "Portals:    R   toggle portal rendering entirely\n"
                 "Stats:      L   log last-frame cull/draw counters\n"
                 "Help:       H or F1   reprint this banner\n"
                 "Pipeline at boot: {}",
                 recursionDepth,
                 useStencilPortals ? "stencil (1.4 B)" : "FBO (1.4 A)");
}

}  // namespace

CastleWindow::CastleWindow(bool useStencilPortals,
                           int initialDepth,
                           const std::string &explicitScenePath)
    : Window(1280, 720),
      useStencilPortals_(useStencilPortals),
      recursionDepth_(initialDepth) {
  const auto exeDir = currentExecutableDir();

  // 1. Mount resources.zip for stock textures / shaders that the rest of
  //    the engine references via `:/textures/...`.
  std::filesystem::path zipPath = exeDir / "resources.zip";
  if (!std::filesystem::exists(zipPath)) {
    zipPath = exeDir.parent_path() / "Demo" / "Resources" / "resources.zip";
    if (!std::filesystem::exists(zipPath)) {
      zipPath = exeDir.parent_path() / "Demo" / "resources.zip";
    }
  }
  fs::instance()->add(zipPath.string());

  // 2. Register the executable dir as an additional disk-overlay root.
  //    This is the Phase-2-demo leg of FileSystem's disk overlay: it lets
  //    the scene JSON reference `:/castle/textures/wall_stone.png` and
  //    resolve to `<exeDir>/castle/textures/wall_stone.png` regardless of
  //    process cwd. Without this the texture lookup only worked when the
  //    binary was launched from inside `bin/`.
  fs::instance()->add(exeDir.string());

  const auto scenePath = findSceneFile(exeDir, explicitScenePath);
  if (scenePath.empty()) {
    OMEGA_LOG_WARN("castle-demo",
                   "No scene JSON found (looked for castle_labyrinth.json, "
                   "castle_scene.json). Window will come up empty.");
    return;
  }
  OMEGA_LOG_INFO("castle-demo", "Loading scene: {}", scenePath.string());

  auto loader = std::make_shared<PortalSceneLoader>();
  scene_ = loader->loadFromFile(scenePath.string());
  if (!scene_) {
    OMEGA_LOG_WARN("castle-demo", "Scene load failed.");
    return;
  }

  portalsById_ = loader->getPortals();

  const auto cameraConfig = loader->getCameraConfig();
  glm::vec3 cameraPos = cameraConfig.position;
  // Keep the camera above the floor no matter what the JSON says.
  cameraPos.y = std::max(cameraPos.y, 1.0f);
  camera_ = std::make_shared<CameraFPS>(cameraPos,
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        cameraConfig.yaw,
                                        cameraConfig.pitch);
  setCamera(camera_);
  camera_->setPerspective(60.0f, 1280.0f, 720.0f, 0.1f, 200.0f);

  if (auto pr = scene_->getPortalRenderer()) {
    pr->setMaxRecursionDepth(recursionDepth_);
    pr->setStencilMode(useStencilPortals_);
    pr->setEnabled(true);
  }

  scene_->add(camera_);
  scene_->setCurrentCamera(0);

  printHelpBanner(useStencilPortals_, recursionDepth_);
}

void CastleWindow::process() {
  if (scene_) scene_->process(m_deltaTime);
  Window::process();
}

bool CastleWindow::render() {
  if (scene_) scene_->render();
  return Window::render();
}

void CastleWindow::keyEvent(int state, int key, int modifier, bool repeat) {
  using namespace omega::input;
  const bool pressed = (state == KEY_STATE_DOWN) && !repeat;
  if (pressed) {
    switch (key) {
      case KEY_ESCAPE: quit(); return;
      case KEY_H:
      case KEY_F1: printHelpBanner(isStencilActive(), recursionDepth_); return;
      case KEY_P: togglePipeline(); return;
      case KEY_1: setRecursionDepth(1); return;
      case KEY_2: setRecursionDepth(2); return;
      case KEY_3: setRecursionDepth(3); return;
      case KEY_4: setRecursionDepth(4); return;
      case KEY_O: cycleClosedDoor(); return;
      case KEY_R: togglePortalRendererEnabled(); return;
      case KEY_L: logRenderStats(); return;
      default: break;
    }
  }
  Window::keyEvent(state, key, modifier, repeat);
}

void CastleWindow::togglePipeline() {
  auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
  if (!pr) return;
  const bool now = !pr->isStencilMode();
  pr->setStencilMode(now);
  useStencilPortals_ = now;
  OMEGA_LOG_INFO("castle-demo", "Portal pipeline -> {}",
                 now ? "stencil (1.4 B)" : "FBO (1.4 A)");
}

void CastleWindow::setRecursionDepth(int depth) {
  auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
  if (!pr) return;
  recursionDepth_ = depth;
  pr->setMaxRecursionDepth(depth);
  OMEGA_LOG_INFO("castle-demo", "Max portal recursion depth -> {}", depth);
}

void CastleWindow::togglePortalRendererEnabled() {
  auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
  if (!pr) return;
  const bool now = !pr->isEnabled();
  pr->setEnabled(now);
  OMEGA_LOG_INFO("castle-demo", "PortalRenderer enabled = {}", now);
}

void CastleWindow::cycleClosedDoor() {
  // Collect the "closed" doors by id prefix. We don't cache this list
  // because the JSON is authored with stable ids and reloading is a
  // rebuild — O(n) on a handful of ids each keypress is fine.
  std::vector<std::string> closed;
  closed.reserve(portalsById_.size());
  for (const auto &[id, _] : portalsById_) {
    if (id.rfind("closed_", 0) == 0) closed.push_back(id);
  }
  std::sort(closed.begin(), closed.end());
  if (closed.empty()) {
    OMEGA_LOG_WARN("castle-demo",
                   "No portals with id prefix 'closed_' in this scene.");
    return;
  }
  const auto &id = closed[closedDoorIndex_ % closed.size()];
  if (auto p = portal(id)) {
    const bool now = !p->isOpen();
    p->setOpen(now);
    OMEGA_LOG_INFO("castle-demo", "Portal '{}' open = {}", id, now);
  }
  closedDoorIndex_ = (closedDoorIndex_ + 1) % closed.size();
}

void CastleWindow::logRenderStats() {
  if (!scene_) return;
  const auto &s = scene_->lastRenderStats();
  OMEGA_LOG_INFO(
      "castle-demo",
      "stats: considered={} drawn={} culledFrustum={} drawnNoBounds={} "
      "clippingActive={}",
      s.considered, s.drawn, s.culledFrustum, s.drawnNoBounds,
      s.clippingActive ? "yes" : "no");
}

std::shared_ptr<Portal> CastleWindow::portal(const std::string &id) const {
  auto it = portalsById_.find(id);
  if (it == portalsById_.end()) return nullptr;
  return it->second;
}

bool CastleWindow::isStencilActive() const {
  auto pr = scene_ ? scene_->getPortalRenderer() : nullptr;
  return pr ? pr->isStencilMode() : useStencilPortals_;
}

}  // namespace omega::demo::castle
