#include <render/DirectionalLight.h>
#include <render/Shader.h>

using namespace omega::render;

void DirectionalLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    std::cout << "Light not used because of MAX_POINT limit" << std::endl;
    return;
  }
  auto no = std::to_string(point_no);
  shader->setVec3("dirLight[" + no + "].direction", direction_);
  shader->setVec3("dirLight[" + no + "].ambient", ambient_);
  shader->setVec3("dirLight[" + no + "].diffuse", diffuse_);
  shader->setVec3("dirLight[" + no + "].specular", specular_);
  shader->setInt("dirLight[" + no + "].on", 1);
}

void DirectionalLight::dump() {
  std::cout << "----------------------" << std::endl;
  std::cout << "dirLight.direction = " << direction_.x << "," << direction_.y
            << "," << direction_.z << std::endl;
  std::cout << "dirLight.ambient" << ambient_.x << "," << ambient_.y << ","
            << ambient_.z << std::endl;
  std::cout << "dirLight.diffuse = " << diffuse_.x << "," << diffuse_.y << ","
            << diffuse_.z << std::endl;
  std::cout << "dirLight.specular = " << specular_.x << "," << specular_.y
            << "," << specular_.z << std::endl;
  std::cout << "dirLight.on = " << 1 << std::endl;
}
