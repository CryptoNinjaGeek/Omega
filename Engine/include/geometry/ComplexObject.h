#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <render/Texture.h>
#include <render/Shader.h>
#include <render/PointLight.h>
#include <render/DirectionalLight.h>
#include <render/SpotLight.h>

#include "interface/ObjectInterface.h"
#include <geometry/Vertex.h>
#include <utils/ObjectGenerator.h>
#include <geometry/Object.h>
#include <geometry/ObjectTree.h>

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

class ComplexObject : public ObjectInterface {
public:
  ComplexObject(bool debug = false);

  auto load(std::string const &path, double scale = 1.f) -> void;
  auto process() -> void override;
  auto prepare(input::ObjectPreparation) -> void override;
  auto setShader(std::shared_ptr<render::Shader> shader) -> void override;
  void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights);

  auto scale(float) -> void override;
  auto translate(glm::vec3) -> void override;
  auto rotate(float, glm::vec3) -> void override;
  auto entityModel() -> glm::mat4 override;

  auto root() -> ObjectNodePtr { return _root; }

private:
  auto prepare(ObjectNodePtr node, input::ObjectPreparation input) -> void;
  auto render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera, glm::mat4 model = glm::mat4(1.0f)) -> void;
  auto process(ObjectNodePtr node) -> void;
  auto shaders(ObjectNodePtr node, std::shared_ptr<render::Shader> shader) -> void;

protected:
  bool debug_{false};
  ObjectNodePtr _root;
};
}  // namespace geometry
}  // namespace omega
