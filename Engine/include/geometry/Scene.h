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

#include "physics/PhysicsEngine.h"

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

enum class FindPattern {
  Exact,
  Contains,
  StartsWith,
};

class Scene {
public:
  // constructor, expects a filepath to a 3D model.
  Scene(bool physics = true, bool gamma = false);
  Scene(std::string const &path,bool physics = true, bool gamma = false);

  auto import(std::string const &path) -> void;

  void render();
  void render(std::shared_ptr<render::Camera> camera);
  void shaders(std::shared_ptr<render::Shader> shader,
			   std::shared_ptr<render::Shader> lightShader,
			   std::shared_ptr<render::Shader> debugShader);
  void lights(std::vector<std::shared_ptr<Light>>);
  auto scale(float) -> void;
  auto process(float) -> void;
  auto prepare() -> void;
  auto debug(bool val) -> void;

  auto findFirst(std::string, FindPattern pattern = FindPattern::Exact ) -> std::shared_ptr<ObjectInterface>;
  auto find(std::string, FindPattern pattern = FindPattern::Exact ) -> std::vector<std::shared_ptr<ObjectInterface>>;


  auto add(std::shared_ptr<ObjectNode> tree) ->void;
  auto add(std::shared_ptr<ObjectInterface> object) ->void;
  auto add(std::shared_ptr<Light> light) -> void { lights_.push_back(light); }
  auto add(std::shared_ptr<Camera> camera) -> unsigned int {
	cameras_.push_back(camera);
	return cameras_.size() - 1;
  }

  auto setCurrentCamera(unsigned int index) -> void;
  auto currentCamera() -> std::shared_ptr<Camera> { return cameras_[current_camera_]; }

private:
  void loadModel(std::string const &path);
  auto prepare(ObjectNodePtr node) -> void;
  auto render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera, glm::mat4 model = glm::mat4(1.0f)) -> void;
  auto process(ObjectNodePtr node) -> void;
  void shaders(std::shared_ptr<render::Shader> shader, ObjectNodePtr node);
  void lights(std::vector<std::shared_ptr<Light>> lights, ObjectNodePtr node);

  auto findFirst(std::string name, ObjectNodePtr node, FindPattern pattern = FindPattern::Exact) -> std::shared_ptr<ObjectInterface>;
  auto find(std::string name, ObjectNodePtr node, FindPattern pattern = FindPattern::Exact) -> std::vector<std::shared_ptr<ObjectInterface>>;

protected:
  ObjectNodePtr _root;

  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Camera>> cameras_;
  std::shared_ptr<Shader> lightShader_;
  std::shared_ptr<Shader> meshShader_;
  std::shared_ptr<Shader> debugShader_;

  std::shared_ptr<ObjectInterface> aabb_{nullptr};

  unsigned int current_camera_{0};
  bool gammaCorrection_{false};
  bool debug_{false};
  bool physics_{false};
};
}  // namespace geometry
}  // namespace omega
