#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <geometry/Point3.h>
#include <render/Window.h>
#include <system/System.h>
#include <render/CameraFPS.h>
#include <render/Shader.h>
#include <render/Material.h>
#include <render/Texture.h>
#include <geometry/Object.h>
#include <system/FileSystem.h>
#include <geometry/Scene.h>
#include <geometry/PortalRenderer.h>
#include <utils/PortalSceneLoader.h>

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::utils;
using namespace omega::interface;
using namespace omega::input;
using namespace omega;

class RoomsWindow : public Window {
public:
  RoomsWindow() : Window(1280, 720) {
    // Get the executable directory
    std::filesystem::path exePath;
    #ifdef __APPLE__
      // On macOS, use _NSGetExecutablePath
      uint32_t size = 0;
      _NSGetExecutablePath(nullptr, &size);
      std::vector<char> path(size);
      _NSGetExecutablePath(path.data(), &size);
      exePath = std::filesystem::canonical(path.data()).parent_path();
    #else
      // On Linux, use /proc/self/exe
      exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
    #endif
    
    // Look for resources.zip in the executable directory
    std::filesystem::path zipPath = exePath / "resources.zip";
    if (!std::filesystem::exists(zipPath)) {
      // Fallback: try Demo directory relative to executable
      zipPath = exePath.parent_path() / "Demo" / "Resources" / "resources.zip";
      if (!std::filesystem::exists(zipPath)) {
        zipPath = exePath.parent_path() / "Demo" / "resources.zip";
      }
    }
    
    fs::instance()->add(zipPath.string());

    // Find scene JSON file
    std::filesystem::path scenePath = exePath / "rooms_scene.json";
    if (!std::filesystem::exists(scenePath)) {
      // Fallback: try Demo/Rooms directory relative to executable
      scenePath = exePath.parent_path() / "Demo" / "Rooms" / "rooms_scene.json";
      if (!std::filesystem::exists(scenePath)) {
        std::cerr << "Error: Could not find rooms_scene.json" << std::endl;
        return;
      }
    }

    // Load scene from JSON
    auto loader = std::make_shared<PortalSceneLoader>();
    scene = loader->loadFromFile(scenePath.string());
    
    if (!scene) {
      std::cerr << "Error: Failed to load scene from JSON" << std::endl;
      return;
    }

    // Get camera configuration from loader
    auto cameraConfig = loader->getCameraConfig();

    // Setup camera
    camera = std::make_shared<CameraFPS>(
        cameraConfig.position,
        glm::vec3(0.0f, 1.0f, 0.0f),
        cameraConfig.yaw,
        cameraConfig.pitch
    );
    setCamera(camera);
    camera->setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

    // Get portal renderer from scene (created by loader)
    auto portalRenderer = scene->getPortalRenderer();
    if (portalRenderer) {
      portalRenderer->setMaxRecursionDepth(2);
    }

    // Add camera to scene
    scene->add(camera);
    scene->setCurrentCamera(0);
  }

  void process() override {
    scene->process(m_deltaTime);
    Window::process();
  }

  bool render() override {
    scene->render();
    return Window::render();
  }


private:
  std::shared_ptr<Scene> scene;
  std::shared_ptr<CameraFPS> camera;
};

int main(int argc, char* argv[]) {
  OSystem::init();

  auto window = std::make_shared<RoomsWindow>();
  Window::setInstance(window);

  std::cout << "=== Rooms Demo ===" << std::endl;
  std::cout << "Use WASD to move, mouse to look around" << std::endl;
  std::cout << "Two rooms connected by a hallway with portals" << std::endl;
  std::cout << "Look through the doorways to see the other rooms" << std::endl;

  while (window->isRuning()) {
    window->process();
    window->clear();
    window->render();
    window->swap();
  }

  return 0;
}

