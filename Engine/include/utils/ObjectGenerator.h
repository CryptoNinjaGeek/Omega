#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <render/CubeTexture.h>
#include <interface/Light.h>
#include <geometry/Vertex.h>
#include <geometry/BoundingBox.h>
#include <optional>
#include <memory>
#include <map>

namespace omega {
namespace geometry {
class Object;
}
namespace render {
class Shader;
class Texture;
class Material;
}  // namespace render
namespace utils {
namespace input {
struct ObjectGenerator {
  std::optional<glm::vec3> position;
  std::optional<glm::mat4x4> matrix;
  std::shared_ptr<omega::render::Shader> shader;
  std::vector<std::shared_ptr<omega::render::Texture>> textures;
  std::optional<omega::render::Material> material;
  std::optional<std::vector<std::shared_ptr<interface::Light>>> lights;
  float size{1.0f};
  float mass{1.f};
  std::string name;
  bool physics{true};
  bool lighting{true};
};
struct MeshInput {
  std::vector<omega::geometry::Vertex> vertices;
  std::vector<unsigned int> indices;
  std::map<std::string, std::shared_ptr<omega::render::Texture>> textures;
  std::string name;
  geometry::BoundingBox boundingBox;
  unsigned int flags{0};
  double scale{1.f};
};
}  // namespace input

enum { ogMirrorUV = 0x1 };

class OMEGA_EXPORT ObjectGenerator {
public:
  static auto box(input::ObjectGenerator)
  -> std::shared_ptr<omega::geometry::Object>;

  static auto plane(input::ObjectGenerator)
  -> std::shared_ptr<omega::geometry::Object>;

  static auto dome(omega::input::CubeTextureInput, float size = 10.f)
  -> std::shared_ptr<omega::geometry::Object>;

  static auto mesh(input::MeshInput)
  -> std::shared_ptr<omega::geometry::Object>;

  static auto container(input::ObjectGenerator)
  -> std::shared_ptr<omega::geometry::Object>;

private:
  static auto mirrorUV(std::vector<omega::geometry::Vertex> &vertices) -> void;
};
}  // namespace utils
}  // namespace omega
