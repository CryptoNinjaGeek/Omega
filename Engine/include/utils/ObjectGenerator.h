#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <render/CubeTexture.h>
#include <interface/Light.h>
#include <optional>
#include <memory>

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
  float size{0.5f};
};
}  // namespace input
class OMEGA_EXPORT ObjectGenerator {
 public:
  static auto box(input::ObjectGenerator)
      -> std::shared_ptr<omega::geometry::Object>;

  static auto plane(input::ObjectGenerator)
      -> std::shared_ptr<omega::geometry::Object>;

  static auto dome(omega::input::CubeTextureInput, float size = 10.f)
      -> std::shared_ptr<omega::geometry::Object>;
};
}  // namespace utils
}  // namespace omega
