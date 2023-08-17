#pragma once

#include <string>

#include <system/Global.h>
#include <interface/Light.h>

namespace omega {
namespace input {
struct DirectionalLightInput {
  glm::vec3 direction;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
};
}  // namespace input
namespace render {

class OMEGA_EXPORT DirectionalLight : public interface::Light {
 public:
  DirectionalLight(input::DirectionalLightInput input)
      : direction_(input.direction),
        ambient_(input.ambient),
        diffuse_(input.diffuse),
        specular_(input.specular) {}

  interface::LightType type() { return interface::LightType::DIRECTIONAL; }

  void setup(std::shared_ptr<render::Shader>);

 private:
  glm::vec3 direction_;
  glm::vec3 ambient_;
  glm::vec3 diffuse_;
  glm::vec3 specular_;
};
};  // namespace render
};  // namespace omega
