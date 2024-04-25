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

#include <geometry/Object.h>
#include <geometry/Vertex.h>
#include <utils/ObjectGenerator.h>
#include <geometry/Object.h>

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

struct ObjectNode {
  std::vector<std::shared_ptr<ObjectNode>> children;
  std::vector<std::shared_ptr<ObjectInterface>> meshes;
  glm::mat4x4 mat;
};

typedef std::shared_ptr<ObjectNode> ObjectNodePtr;

class ObjectTree {
public:
  ObjectTree();

  auto import(std::string const &path) -> void;

  void render(std::shared_ptr<render::Camera>);
  auto process() -> void;

protected:
  ObjectNodePtr _root;
};
}  // namespace geometry
}  // namespace omega
