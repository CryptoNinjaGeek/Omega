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
