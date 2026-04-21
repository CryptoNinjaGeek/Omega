#pragma once

#include <system/Global.h>
#include <geometry/Scene.h>
#include <geometry/Portal.h>
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/DirectionalLight.h>
#include <render/PointLight.h>
#include <render/SpotLight.h>
#include <render/Material.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace omega {
namespace utils {

/**
 * PortalSceneLoader - Loads portal scenes from JSON format
 * 
 * JSON Format Structure:
 * {
 *   "version": "1.0",
 *   "scene": {
 *     "name": "Scene Name",
 *     "camera": { "position": [x, y, z], "yaw": 0, "pitch": 0 },
 *     "ambient": [r, g, b, a],
 *     "objects": [...],
 *     "portals": [...],
 *     "lights": [...],
 *     "materials": [...],
 *     "textures": [...]
 *   }
 * }
 */
class OMEGA_EXPORT PortalSceneLoader {
public:
  PortalSceneLoader();
  ~PortalSceneLoader() = default;

  /**
   * Load a portal scene from JSON file
   * @param filePath Path to JSON file
   * @return Scene object with portals, objects, and lights configured
   */
  std::shared_ptr<geometry::Scene> loadFromFile(const std::string& filePath);

  /**
   * Load a portal scene from JSON string
   * @param jsonString JSON content as string
   * @return Scene object with portals, objects, and lights configured
   */
  std::shared_ptr<geometry::Scene> loadFromString(const std::string& jsonString);

  /**
   * Get all standalone portals keyed by their JSON `id`.
   *
   * Returned by value (it's a map of shared_ptr — cheap) so callers can
   * look up named portals and tweak them at runtime (e.g. demos toggling
   * mirror overlays or opening a closed doorway on a hotkey). The scene
   * format is now exclusively the doorway-based model: each portal declares
   * its own `destination` field (or omits it to be non-traversable).
   */
  std::map<std::string, std::shared_ptr<geometry::Portal>> getPortals() const {
    return portals_;
  }

  /**
   * Get initial camera position/orientation
   */
  struct CameraConfig {
    glm::vec3 position{0.0f, 1.5f, 5.0f};
    float yaw{-90.0f};
    float pitch{0.0f};
  };
  CameraConfig getCameraConfig() const { return cameraConfig_; }

private:
  void parseScene(const nlohmann::json& json);
  void parseObjects(const nlohmann::json& json, geometry::Scene* scene, 
                    std::shared_ptr<render::Shader> defaultShader);
  void parsePortals(const nlohmann::json& json);
  void parseLights(const nlohmann::json& json, geometry::Scene* scene);
  void parseMaterials(const nlohmann::json& json);
  void parseTextures(const nlohmann::json& json);
  void parseCamera(const nlohmann::json& json);

  std::map<std::string, std::shared_ptr<geometry::Portal>> portals_;
  std::map<std::string, std::shared_ptr<render::Texture>> textures_;
  std::map<std::string, render::Material> materials_;
  CameraConfig cameraConfig_;
  
  // Default white texture for objects with materials but no textures
  std::shared_ptr<render::Texture> defaultWhiteTexture_;
  
  // Get or create default white texture
  std::shared_ptr<render::Texture> getDefaultWhiteTexture();
  
  // Helper methods for JSON parsing
  glm::vec3 parseVec3(const nlohmann::json& json, const std::string& key, glm::vec3 defaultValue = glm::vec3(0.0f));
  glm::vec4 parseVec4(const nlohmann::json& json, const std::string& key, glm::vec4 defaultValue = glm::vec4(0.0f));
  float parseFloat(const nlohmann::json& json, const std::string& key, float defaultValue = 0.0f);
  std::string parseString(const nlohmann::json& json, const std::string& key, const std::string& defaultValue = "");
  bool parseBool(const nlohmann::json& json, const std::string& key, bool defaultValue = false);
};

}  // namespace utils
}  // namespace omega

