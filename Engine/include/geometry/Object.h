#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <interface/Entity.h>
#include <interface/Light.h>
#include <memory>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {

class OMEGA_EXPORT Object : public interface::Entity {
 public:
  Object(unsigned int vao, unsigned int vbo, unsigned int cnt);
  Object();

  virtual void render(std::shared_ptr<render::Camera>);

  void setMaterial(render::Material material) { material_ = material; }
  void setPosition(glm::vec3);
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

  // Entity functions
  glm::vec3 entityPosition() { return glm::vec3(model_[3]); }

 protected:
  unsigned int vao_;
  unsigned int vbo_;
  unsigned int count_;

  glm::mat4 model_;
  std::optional<render::Material> material_;
  std::shared_ptr<render::Shader> shader_;
  std::vector<std::shared_ptr<render::Texture>> textures_;
  std::vector<std::shared_ptr<interface::Light>> lights_;
};
}  // namespace geometry
}  // namespace omega
