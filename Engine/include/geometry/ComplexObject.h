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

class ComplexObject : public ObjectInterface {
public:
  ComplexObject();

  auto load(std::string const &path) -> void;
  auto render(std::shared_ptr<render::Camera>, glm::mat4) -> void override;
  auto process() -> void override;
  auto prepare(input::ObjectPreparation) -> void override;
  auto setShader(std::shared_ptr<render::Shader> shader) -> void override;
  void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights);

  auto scale(float) -> void override;
  auto translate(glm::vec3) -> void override;
  auto rotate(float, glm::vec3) -> void override;

  glm::vec3 entityPosition() override;
  
private:
  auto prepare(ObjectNodePtr node, input::ObjectPreparation input) -> void;
  auto render(ObjectNodePtr node, glm::mat4, std::shared_ptr<render::Camera> camera) -> void;
  auto process(ObjectNodePtr node) -> void;
  auto shaders(ObjectNodePtr node, std::shared_ptr<render::Shader> shader) -> void;

protected:
  ObjectNodePtr _root;
};
}  // namespace geometry
}  // namespace omega
