#pragma once

#include "system/Global.h"
#include "render/Material.h"
#include "geometry/Entity.h"
#include "Light.h"
#include "physics/PhysicsObjectInput.h"
#include <memory>
#include <utility>
#include <vector>
#include <optional>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace input {
struct ObjectPreparation {
  std::vector<std::shared_ptr<interface::Light>> lights;
  glm::mat4 model;
};
}  // namespace input

namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {
enum class RenderType { Elements, Array };
enum class ObjectType { ComplexObject, Object, SkyBox };

struct ObjectData {
  std::string name_;
  unsigned int vao_;
  unsigned int vbo_;
  unsigned int count_;
  RenderType render_type_{RenderType::Array};
  ObjectType type_{ObjectType::Object};

  glm::mat4 model_;

  bool visible_{true};
  bool debug_{false};

  std::optional<render::Material> material_;
  std::shared_ptr<render::Shader> shader_;
  std::vector<std::shared_ptr<render::Texture>> textures_;
  std::vector<std::shared_ptr<interface::Light>> lights_;
};

class OMEGA_EXPORT ObjectInterface : public interface::Entity {
public:
  ObjectInterface() {
	data_ = std::make_shared<ObjectData>();
  };

  virtual ~ObjectInterface() {
	data_.reset();
  }

  virtual auto process() -> void = 0;
  virtual auto prepare(input::ObjectPreparation) -> void = 0;
  virtual void setShader(std::shared_ptr<render::Shader> shader) = 0;
  virtual void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) = 0;

  virtual auto scale(float) -> void = 0;
  virtual auto translate(glm::vec3) -> void = 0;
  virtual auto rotate(float, glm::vec3) -> void = 0;

  virtual auto setMaterial(render::Material material) -> void = 0;
  virtual auto material() -> std::optional<render::Material> = 0;

  virtual auto textures() -> std::vector<std::shared_ptr<render::Texture>> = 0;
  virtual auto lights() -> std::vector<std::shared_ptr<interface::Light>> = 0;

  virtual auto shader() -> std::shared_ptr<render::Shader> = 0;

  void setName(std::string name) { data_->name_ = std::move(name); }
  auto name() -> std::string { return data_->name_; }

  inline auto visible(bool val) -> void { data_->visible_ = val; }
  inline auto visible() -> bool { return data_->visible_; }

  virtual auto debug(bool val) -> void { data_->debug_ = val; }
  auto debug() -> bool { return data_->debug_; }

  auto model() -> glm::mat4 { return data_->model_; };
  auto model(glm::mat4 model) -> void { data_->model_ = model; }

  auto data() -> std::shared_ptr<ObjectData> { return data_; }
  auto type() -> ObjectType { return data_->type_; }

protected:
  std::shared_ptr<ObjectData> data_;
};
}  // namespace geometry
}  // namespace omega
