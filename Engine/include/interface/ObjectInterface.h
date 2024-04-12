#pragma once

#include "system/Global.h"
#include "render/Material.h"
#include "Entity.h"
#include "Light.h"
#include "system/PhysicsObject.h"
#include <memory>
#include <vector>
#include <optional>

#include "glm/gtc/matrix_transform.hpp"
#include "reactphysics3d/reactphysics3d.h"

namespace omega {
namespace input {
struct ObjectPreparation {
  std::vector<std::shared_ptr<interface::Light>> lights;
  reactphysics3d::PhysicsWorld *physics_world;
  reactphysics3d::PhysicsCommon *physics_common;
};
}  // namespace input

namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {
enum class ObjectType { Elements, Array };

class OMEGA_EXPORT ObjectInterface : public interface::Entity {
public:
  ObjectInterface() = default;

  virtual void render(std::shared_ptr<render::Camera>, glm::mat4 mat = glm::mat4(1.0f)) = 0;
  virtual auto process() -> void = 0;
  virtual auto prepare(input::ObjectPreparation) -> void = 0;
  virtual void setShader(std::shared_ptr<render::Shader> shader) = 0;
  virtual void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) = 0;

  virtual auto scale(float) -> void = 0;
  virtual auto translate(glm::vec3) -> void = 0;
  virtual auto rotate(float, glm::vec3) -> void = 0;

  void setName(std::string name) { name_ = name; }
  auto name() -> std::string { return name_; }

  inline auto visible(bool val) -> void { visible_ = val; }
  inline auto visible() -> bool { return visible_; }

protected:
  std::string name_;
  bool visible_{true};
};
}  // namespace geometry
}  // namespace omega
