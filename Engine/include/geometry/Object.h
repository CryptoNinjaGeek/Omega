#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <interface/Entity.h>
#include <interface/Light.h>
#include <system/PhysicsObject.h>
#include <memory>
#include <vector>
#include <optional>

#include "glm/gtc/matrix_transform.hpp"
#include <reactphysics3d/reactphysics3d.h>

namespace omega {
namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {

enum class ObjectType { Elements, Array };

class OMEGA_EXPORT Object : public interface::Entity {
public:
  Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
		 ObjectType type = ObjectType::Array);
  Object();

  virtual void render(std::shared_ptr<render::Camera>);
  virtual auto setupPhysics(reactphysics3d::PhysicsWorld *, reactphysics3d::PhysicsCommon *) -> void;
  virtual auto process() -> void;

  auto debug(bool) -> void;

  void setName(std::string name) { name_ = name; }
  void setMaterial(render::Material material) { material_ = material; }
  void setModel(glm::mat4x4 mat) { model_ = mat; }
  void setShader(std::shared_ptr<render::Shader> shader) { shader_ = shader; }
  void addTexture(std::shared_ptr<render::Texture> texture) {
	textures_.push_back(texture);
  };
  void setTextures(std::vector<std::shared_ptr<render::Texture>> textures) {
	textures_ = textures;
  };

  void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) {
	lights_ = lights;
  }

  glm::vec3 entityPosition() { return glm::vec3(model_[3]); }

  auto name() -> std::string { return name_; }
  auto scale(float) -> void;
  auto position(glm::vec3) -> void;
  auto physics(physics::PhysicsObject physicsObject) -> void { physicsObject_ = physicsObject; }
  inline auto visible(bool val) -> void { visible_ = val; }
  inline auto visible() -> bool { return visible_; }

protected:
  std::string name_;
  unsigned int vao_;
  unsigned int vbo_;
  unsigned int count_;
  ObjectType type_{ObjectType::Array};

  reactphysics3d::RigidBody *body_{nullptr};
  physics::PhysicsObject physicsObject_;
  reactphysics3d::Collider *collider_{nullptr};

  bool visible_{true};
  glm::mat4 model_;
  std::optional<render::Material> material_;
  std::shared_ptr<render::Shader> shader_;
  std::vector<std::shared_ptr<render::Texture>> textures_;
  std::vector<std::shared_ptr<interface::Light>> lights_;
};
}  // namespace geometry
}  // namespace omega
