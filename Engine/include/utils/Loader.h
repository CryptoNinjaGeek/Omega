#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <render/CubeTexture.h>
#include <geometry/ObjectTree.h>
#include <interface/Light.h>
#include <geometry/Vertex.h>
#include <optional>
#include <memory>
#include <map>
#include <assimp/material.h>

class aiNode;
class aiScene;
class aiMaterial;
class aiMesh;

namespace omega {
namespace geometry {
class Object;
}
namespace render {
class Texture;
}
namespace utils {
namespace input {
struct LoaderInput {
  std::string path;
  double scale{1.f};
  bool debug{false};
};
}

class OMEGA_EXPORT Loader {
public:
  static auto loadModel(input::LoaderInput) -> ObjectNodePtr;

private:
  static auto processNode(aiNode *node, const aiScene *scene, input::LoaderInput) -> ObjectNodePtr;
  static std::shared_ptr<geometry::Object> processMesh(std::string name, aiMesh *mesh,
													   const aiScene *scene, input::LoaderInput, double scale = 1.f);
  static std::map<std::string, std::shared_ptr<render::Texture>> loadMaterialTextures(aiMaterial *mat,
																					  aiTextureType type,
																					  std::string typeName);
private:
  static glm::vec3 preprocess(glm::vec3);
  static glm::mat4 preprocess(glm::mat4);
  static void processAnimations(const aiScene *animations, ObjectNodePtr tree, input::LoaderInput);
};
}  // namespace utils
}  // namespace omega
