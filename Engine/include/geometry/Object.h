#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <interface/ObjectInterface.h>
#include <interface/Light.h>
#include <memory>
#include <vector>
#include <optional>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {

class OMEGA_EXPORT Object : public ObjectInterface {
  friend class OpenGLRender;
public:
  Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
		 RenderType type = RenderType::Array);
  Object();

  virtual auto process() -> void override;
  virtual auto prepare(input::ObjectPreparation) -> void override;
  virtual auto setShader(std::shared_ptr<render::Shader> shader) -> void override;

  auto shader() -> std::shared_ptr<render::Shader> override { return data_->shader_; }

  auto scale(float) -> void override;
  auto translate(glm::vec3) -> void override;
  auto rotate(float, glm::vec3) -> void override;

  auto setMaterial(render::Material material) -> void { data_->material_ = material; }
  auto material() -> std::optional<render::Material> override { return data_->material_; }

  void setModel(glm::mat4x4 mat) { data_->model_ = mat; }
  void addTexture(std::shared_ptr<render::Texture> texture) {
	data_->textures_.push_back(texture);
  };
  void setTextures(std::vector<std::shared_ptr<render::Texture>> textures) {
	data_->textures_ = textures;
  };

  void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) override {
	data_->lights_ = lights;
  }

  auto textures() -> std::vector<std::shared_ptr<render::Texture>> override { return data_->textures_; }
  auto lights() -> std::vector<std::shared_ptr<interface::Light>> override { return data_->lights_; }

  auto position(glm::vec3) -> void;
};
}  // namespace geometry
}  // namespace omega
