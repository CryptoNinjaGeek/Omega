#include <render/SpotLight.h>
#include <render/Shader.h>
#include <interface/Entity.h>

using namespace omega::render;

void SpotLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    std::cout << "Light not used because of MAX_POINT limit" << std::endl;
    return;
  }
  auto no = std::to_string(point_no);

  if (tracking_) {
    position_ = tracking_->get()->entityPosition();
    direction_ = tracking_->get()->entityDirection();
  }

  shader->setVec3("spotLight[" + no + "].position", position_);
  shader->setVec3("spotLight[" + no + "].direction", direction_);
  shader->setVec3("spotLight[" + no + "].ambient", ambient_);
  shader->setVec3("spotLight[" + no + "].diffuse", diffuse_);
  shader->setVec3("spotLight[" + no + "].specular", specular_);
  shader->setFloat("spotLight[" + no + "].constant", constant_);
  shader->setFloat("spotLight[" + no + "].linear", linear_);
  shader->setFloat("spotLight[" + no + "].quadratic", quadratic_);
  shader->setFloat("spotLight[" + no + "].cutOff", cutOff_);
  shader->setFloat("spotLight[" + no + "].outerCutOff", outerCutOff_);
  shader->setInt("spotLight[" + no + "].on", 1);
}
