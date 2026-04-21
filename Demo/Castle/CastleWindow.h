#pragma once

// CastleWindow — the window subclass that drives the Castle labyrinth POC.
//
// The Castle demo is intentionally larger than Demo/Portal's single-room
// showcase: ~15 rooms connected by through-portal doorways, exercising
// the Phase 1 FBO + stencil portal pipelines at scale. Every doorway is
// a through-portal; a handful have `open=false` to simulate the "sliding
// down from the ceiling" doors we will animate in a later phase.
//
// Scope deliberately narrow: load a JSON scene, route WASD+mouse look to
// the base Window, wire a thin hotkey panel (pipeline toggle, recursion
// depth, portal-renderer enable, stats dump, help). No gameplay, no
// physics beyond the static colliders the loader emits for the floor.

#include <map>
#include <memory>
#include <string>

#include <geometry/Scene.h>
#include <geometry/Portal.h>
#include <render/CameraFPS.h>
#include <render/Window.h>

namespace omega::demo::castle {

class CastleWindow : public omega::render::Window {
 public:
  // `useStencilPortals` — if true, boot with stencil-based portal pipeline
  //                       (Phase 1.4 option B) instead of FBO (option A).
  // `initialDepth`      — starting max recursion depth; changeable at
  //                       runtime via the 1/2/3/4 hotkeys.
  // `explicitScenePath` — optional `--scene <path>` override. When empty
  //                       we search for `castle_labyrinth.json` first in
  //                       the executable directory, then in the source
  //                       tree under Demo/Castle/.
  CastleWindow(bool useStencilPortals,
               int initialDepth,
               const std::string &explicitScenePath);

  void process() override;
  bool render() override;

  void keyEvent(int state, int key, int modifier, bool repeat) override;

 private:
  // Hotkey actions — one method per key so adding or renaming one doesn't
  // require rewriting the switch block in keyEvent.
  void togglePipeline();
  void setRecursionDepth(int depth);
  void togglePortalRendererEnabled();
  void cycleClosedDoor();   // Toggle the "currently focused" closed door
  void logRenderStats();

  std::shared_ptr<omega::geometry::Portal> portal(const std::string &id) const;
  bool isStencilActive() const;

  std::shared_ptr<omega::geometry::Scene> scene_;
  std::shared_ptr<omega::render::CameraFPS> camera_;
  std::map<std::string, std::shared_ptr<omega::geometry::Portal>> portalsById_;

  bool useStencilPortals_{false};
  int  recursionDepth_{2};
  // Index into the list of portals whose id begins with `closed_` — cycled
  // through by the `O` hotkey so a tester can open/close each closed door
  // in turn without a dedicated key per door.
  int  closedDoorIndex_{0};
};

}  // namespace omega::demo::castle
