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

void PointLight::dump() {
  std::cout << "----------------------" << std::endl;
  std::cout << "pointLights.position = " << position_.x << "," << position_.y
            << "," << position_.z << std::endl;
  std::cout << "pointLights.ambient = " << ambient_.x << "," << ambient_.y
            << "," << ambient_.z << std::endl;
  std::cout << "pointLights.diffuse = " << diffuse_.x << "," << diffuse_.y
            << "," << diffuse_.z << std::endl;
  std::cout << "pointLights.specular = " << specular_.x << "," << specular_.y
            << "," << specular_.z << std::endl;
  std::cout << "pointLights.constant = " << constant_ << std::endl;
  std::cout << "pointLights.linear = " << linear_ << std::endl;
  std::cout << "pointLights.quadratic = " << quadratic_ << std::endl;

  std::cout << "pointLights.on = " << 1 << std::endl;
}

void PointLight::render(std::shared_ptr<Camera>,
                        std::shared_ptr<render::Shader> shader) {
  shader->use();
}
