#include <render/SpotLight.h>
#include <render/Shader.h>
#include "geometry/Entity.h"

using namespace omega::render;

void SpotLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no==-1) {
	std::cout << "Light not used because of MAX_POINT limit" << std::endl;
	return;
  }
  auto no = std::to_string(point_no);

  if (follow_) {
	position_ = follow_->entityPosition();
	direction_ = follow_->entityDirection();
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
  std::cout << "----------------------" << std::endl;
  std::cout << "spotLight.direction = " << direction_.x << "," << direction_.y
			<< "," << direction_.z << std::endl;
  std::cout << "spotLight.ambient = " << ambient_.x << "," << ambient_.y << ","
			<< ambient_.z << std::endl;
  std::cout << "spotLight.diffuse = " << diffuse_.x << "," << diffuse_.y << ","
			<< diffuse_.z << std::endl;
  std::cout << "spotLight.specular = " << specular_.x << "," << specular_.y
			<< "," << specular_.z << std::endl;
  std::cout << "spotLight.on = " << 1 << std::endl;
}
