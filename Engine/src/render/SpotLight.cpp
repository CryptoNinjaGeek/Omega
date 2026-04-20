#include <render/SpotLight.h>
#include <render/Shader.h>
#include <interface/Entity.h>
#include <system/Log.h>

using namespace omega::render;

void SpotLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    OMEGA_LOG_WARN("light", "SpotLight dropped: MAX_POINT limit reached");
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

void SpotLight::dump() {
  OMEGA_LOG_DEBUG("light",
                  "SpotLight: dir=({},{},{}) ambient=({},{},{}) "
                  "diffuse=({},{},{}) specular=({},{},{}) on=1",
                  direction_.x, direction_.y, direction_.z, ambient_.x,
                  ambient_.y, ambient_.z, diffuse_.x, diffuse_.y, diffuse_.z,
                  specular_.x, specular_.y, specular_.z);
}
