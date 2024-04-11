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

class OMEGA_EXPORT Loader {
public:
  static auto loadModel(std::string path) -> ObjectNodePtr;

private:
  static auto processNode(aiNode *node, const aiScene *scene) -> ObjectNodePtr;
  static std::shared_ptr<geometry::Object> processMesh(std::string name, aiMesh *mesh,
													   const aiScene *scene);
  static std::map<std::string, std::shared_ptr<render::Texture>> loadMaterialTextures(aiMaterial *mat,
																					  aiTextureType type,
																					  std::string typeName);

private:
};
}  // namespace utils
}  // namespace omega
