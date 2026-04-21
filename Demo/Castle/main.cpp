// Castle demo entry point. The real logic lives in CastleWindow so the
// same class can be smoke-tested from a unit harness without spinning up
// GLFW/GLAD main loop machinery.

#include <iostream>
#include <memory>
#include <string>

#include "CastleWindow.h"

#include <render/Window.h>
#include <system/System.h>

using omega::render::Window;
using omega::system::OSystem;
using omega::demo::castle::CastleWindow;

int main(int argc, char *argv[]) {
  OSystem::init();

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
      std::cout << "Usage: CastleDemo [--stencil-portals|-s] [--fbo-portals] "
                   "[--scene <path>] [--depth N]\n"
                   "Hotkeys print on startup; press H or F1 in-game.\n";
      return 0;
    }
  }

  auto window = std::make_shared<CastleWindow>(useStencilPortals,
                                               initialDepth,
                                               explicitScenePath);
  Window::setInstance(window);

  while (window->isRuning()) {
    window->process();
    window->clear();
    window->render();
    window->swap();
  }

  return 0;
}
