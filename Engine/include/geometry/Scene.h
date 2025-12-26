#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <render/Texture.h>
#include <render/Shader.h>
#include <render/PointLight.h>
#include <render/DirectionalLight.h>
#include <render/SpotLight.h>

#include <geometry/ObjectTree.h>
#include <geometry/Object.h>
#include <geometry/Vertex.h>
#include <utils/ObjectGenerator.h>

#include <reactphysics3d/reactphysics3d.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace omega::render;
using namespace omega::interface;

namespace omega {
namespace geometry {

// Forward declaration to break circular dependency
class PortalRenderer;

class Scene {
public:
  // constructor, expects a filepath to a 3D model.
  explicit Scene(bool gamma = false);
  explicit Scene(std::string const &path, bool gamma = false);

  auto import(std::string const &path) -> void;

  void render();
  void render(std::shared_ptr<render::Camera> camera);
  void shaders(std::shared_ptr<render::Shader> shader,
			   std::shared_ptr<render::Shader> lightShader);
  void lights(std::vector<std::shared_ptr<Light>>);
  auto object(std::string) -> std::shared_ptr<Object>;
  auto scale(float) -> void;
  auto process(float) -> void;
  auto prepare() -> void;
  auto debug(bool val) -> void;

  auto add(std::shared_ptr<ObjectNode> tree) ->void;
  auto add(std::shared_ptr<Object> object) ->void;
  auto add(std::shared_ptr<Light> light) -> void { lights_.push_back(light); }
  auto add(std::shared_ptr<Camera> camera) -> unsigned int {
	cameras_.push_back(camera);
	return cameras_.size() - 1;
  }

  auto setCurrentCamera(unsigned int index) -> void;
  auto currentCamera() -> std::shared_ptr<Camera> { return cameras_[current_camera_]; }
  
  // Portal rendering support
  void setPortalRenderer(std::shared_ptr<PortalRenderer> renderer) { portalRenderer_ = renderer; }
  std::shared_ptr<PortalRenderer> getPortalRenderer() const { return portalRenderer_; }
private:
  void loadModel(std::string const &path);
  auto prepare(ObjectNodePtr node) -> void;
  auto render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera) -> void;
  auto process(ObjectNodePtr node) -> void;
  auto object(std::string name, ObjectNodePtr node) -> std::shared_ptr<Object>;
  void shaders(std::shared_ptr<render::Shader> shader, ObjectNodePtr node);
  void lights(std::vector<std::shared_ptr<Light>> lights, ObjectNodePtr node);

protected:
  ObjectNodePtr _root;

  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Camera>> cameras_;
  std::shared_ptr<Shader> lightShader_;
  std::shared_ptr<Shader> meshShader_;

  unsigned int current_camera_{0};

  // logging and memory management
  reactphysics3d::PhysicsCommon physics_common_;

  // Create a physics world
  reactphysics3d::PhysicsWorld *physics_world_;

  bool gammaCorrection{false};
  bool debug_{false};
  
  // Portal rendering
  std::shared_ptr<PortalRenderer> portalRenderer_{nullptr};
};
}  // namespace geometry
}  // namespace omega
