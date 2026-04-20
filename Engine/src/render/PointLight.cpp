#include <render/PointLight.h>
#include <render/Shader.h>
#include <system/Log.h>

using namespace omega::render;

void PointLight::setup(std::shared_ptr<render::Shader> shader) {
  auto point_no = shader->getLightNumber(type());
  if (point_no == -1) {
    OMEGA_LOG_WARN("light", "PointLight dropped: MAX_POINT limit reached");
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
  OMEGA_LOG_DEBUG("light",
                  "PointLight: pos=({},{},{}) ambient=({},{},{}) "
                  "diffuse=({},{},{}) specular=({},{},{}) "
                  "k_const={} k_lin={} k_quad={} on=1",
                  position_.x, position_.y, position_.z, ambient_.x, ambient_.y,
                  ambient_.z, diffuse_.x, diffuse_.y, diffuse_.z, specular_.x,
                  specular_.y, specular_.z, constant_, linear_, quadratic_);
}

void PointLight::render(std::shared_ptr<Camera>,
                        std::shared_ptr<render::Shader> shader) {
  shader->use();
}
