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

class PortalWindow : public Window {
public:
  PortalWindow() : Window(1280, 720) {
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

    // Find scene JSON file (try tunnel_scene.json first, fallback to portal_scene.json)
    std::filesystem::path scenePath = exePath / "tunnel_scene.json";
    if (!std::filesystem::exists(scenePath)) {
      scenePath = exePath / "portal_scene.json";
      if (!std::filesystem::exists(scenePath)) {
        // Fallback: try Demo/Portal directory relative to executable
        scenePath = exePath.parent_path() / "Demo" / "Portal" / "tunnel_scene.json";
        if (!std::filesystem::exists(scenePath)) {
          scenePath = exePath.parent_path() / "Demo" / "Portal" / "portal_scene.json";
          if (!std::filesystem::exists(scenePath)) {
            std::cerr << "Error: Could not find tunnel_scene.json or portal_scene.json" << std::endl;
            return;
          }
        }
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
    
    // Setup camera - adjust Y position to be closer to floor to prevent falling
    glm::vec3 cameraPos = cameraConfig.position;
    cameraPos.y = 1.0f;  // Lower the camera to prevent falling through floor
    
    camera = std::make_shared<CameraFPS>(
        cameraPos,
        glm::vec3(0.0f, 1.0f, 0.0f),
        cameraConfig.yaw,
        cameraConfig.pitch
    );
    setCamera(camera);
    camera->setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

    // Get portal renderer from scene (created by loader)
    auto portalRenderer = scene->getPortalRenderer();
    if (portalRenderer) {
      portalRenderer->setMaxRecursionDepth(2);  // Set recursion depth
    }

    // Add camera to scene
    scene->add(camera);
    scene->setCurrentCamera(0);  // This calls setupPhysics on the camera
  }

  void process() override {
    // Update physics and process scene objects first
    scene->process(m_deltaTime);
    // Then call Window::process() which handles input and camera updates
    Window::process();
  }

  bool render() override {
    // Render scene (includes portal rendering via PortalRenderer)
    scene->render();

    return Window::render();
  }

  void keyEvent(int state, int key, int modifier, bool repeat) override{
	switch (key) {
	case KEY_ESCAPE:
	  if (state==KEY_STATE_DOWN)
		quit();
	  break;
	default:Window::keyEvent(state, key, modifier, repeat);
	  break;
	}
  }


private:
  std::shared_ptr<Scene> scene;
  std::shared_ptr<CameraFPS> camera;
};

int main(int argc, char* argv[]) {
  OSystem::init();

  auto window = std::make_shared<PortalWindow>();
  Window::setInstance(window);

  std::cout << "=== Portal Demo ===" << std::endl;
  std::cout << "Use WASD to move, mouse to look around" << std::endl;
  std::cout << "Portals are set up on left and right walls" << std::endl;
  std::cout << "Scene loaded from JSON file" << std::endl;

  while (window->isRuning()) {
    window->process();
    window->clear();
    window->render();
    window->swap();
  }

  return 0;
}

