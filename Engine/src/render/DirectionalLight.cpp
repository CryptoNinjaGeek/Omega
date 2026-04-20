#include <render/DirectionalLight.h>
#include <render/Shader.h>
#include <system/Log.h>

using namespace omega::render;

void DirectionalLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    OMEGA_LOG_WARN("light", "DirectionalLight dropped: MAX_POINT limit reached");
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
  OMEGA_LOG_DEBUG("light",
                  "DirectionalLight: dir=({},{},{}) ambient=({},{},{}) "
                  "diffuse=({},{},{}) specular=({},{},{}) on=1",
                  direction_.x, direction_.y, direction_.z, ambient_.x,
                  ambient_.y, ambient_.z, diffuse_.x, diffuse_.y, diffuse_.z,
                  specular_.x, specular_.y, specular_.z);
}
