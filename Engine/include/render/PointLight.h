#pragma once

#include <string>

#include <system/Global.h>
#include <interface/Light.h>

namespace omega {
namespace input {
struct PointLightInput {
  glm::vec3 position;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float constant{1.0f};
  float linear{0.9f};
  float quadratic{0.032f};
};
}  // namespace input
namespace render {
class OMEGA_EXPORT PointLight : public interface::Light {
 public:
  PointLight(input::PointLightInput input)
      : position_(input.position),
        ambient_(input.ambient),
        diffuse_(input.diffuse),
        specular_(input.specular),
        constant_(input.constant),
        linear_(input.linear),
        quadratic_(input.quadratic) {}

  interface::LightType type() { return interface::LightType::POINT; }
  void setup(std::shared_ptr<render::Shader>);
  void dump();
  void render(std::shared_ptr<render::Camera>, std::shared_ptr<render::Shader>);

 private:
  glm::vec3 position_;
  glm::vec3 ambient_;
  glm::vec3 diffuse_;
  glm::vec3 specular_;
  float constant_;
  float linear_;
  float quadratic_;
};
};  // namespace render
};  // namespace omega
