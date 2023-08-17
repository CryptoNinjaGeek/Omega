#include <render/PointLight.h>
#include <render/Shader.h>

using namespace omega::render;

void PointLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    std::cout << "Light not used because of MAX_POINT limit" << std::endl;
    return;
  }
  auto no = std::to_string(point_no);
  shader->setVec3("pointLights[" + no + "].position", position_);
  shader->setVec3("pointLights[" + no + "].ambient", ambient_);
  shader->setVec3("pointLights[" + no + "].diffuse", diffuse_);
  shader->setVec3("pointLights[" + no + "].specular", specular_);
  shader->setFloat("pointLights[" + no + "].constant", constant_);
  shader->setFloat("pointLights[" + no + "].linear", linear_);
  shader->setFloat("pointLights[" + no + "].quadratic", quadratic_);
  shader->setInt("pointLights[" + no + "].on", 1);
}
