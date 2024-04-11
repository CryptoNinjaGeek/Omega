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

class ComplexObject {
public:
  ComplexObject();

  auto import(std::string const &path) -> void;

  void render(std::shared_ptr<render::Camera>);
  auto setupPhysics(reactphysics3d::PhysicsWorld *, reactphysics3d::PhysicsCommon *) -> void;
  auto process() -> void;

protected:
  std::vector<std::shared_ptr<Object>> meshes;
};
}  // namespace geometry
}  // namespace omega
