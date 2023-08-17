#pragma once

#include <string>

#include <system/Global.h>
#include <interface/Light.h>

namespace omega {
namespace input {
struct SpotLightInput {
  std::optional<std::shared_ptr<interface::Entity>> tracking;
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float constant;
  float linear;
  float quadratic;
  float cutOff;
  float outerCutOff;
};
}  // namespace input
namespace render {

class OMEGA_EXPORT SpotLight : public interface::Light {
 public:
  SpotLight(input::SpotLightInput input)
      : tracking_(input.tracking),
        position_(input.position),
        direction_(input.direction),
        ambient_(input.ambient),
        diffuse_(input.diffuse),
        specular_(input.specular),
        constant_(input.constant),
        linear_(input.linear),
        quadratic_(input.quadratic),
        cutOff_(input.cutOff),
        outerCutOff_(input.outerCutOff) {}

  interface::LightType type() { return interface::LightType::SPOT; }

  void setup(std::shared_ptr<render::Shader>);

 private:
  std::optional<std::shared_ptr<interface::Entity>> tracking_;
  glm::vec3 position_;
  glm::vec3 direction_;
  glm::vec3 ambient_;
  glm::vec3 diffuse_;
  glm::vec3 specular_;
  float constant_;
  float linear_;
  float quadratic_;
  float cutOff_;
  float outerCutOff_;
};
};  // namespace render
};  // namespace omega
