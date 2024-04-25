#pragma once

#include  <system/Global.h>
#include <geometry/ComplexObject.h>
#include <geometry/Object.h>
#include <geometry/SkyBox.h>
#include <geometry/BoundingBox.h>
#include <interface/Light.h>

namespace omega {
namespace render {
class OMEGA_EXPORT OpenGLRender {
public:
  static std::shared_ptr<OpenGLRender> instance();

public:

  auto camera(std::shared_ptr<render::Camera> camera) -> void { camera_ = camera; }

  auto render(const geometry::BoundingBox &) -> void;
  auto render(std::shared_ptr<geometry::ObjectInterface>) -> void;
  auto render(std::shared_ptr<geometry::ComplexObject>) -> void;
  auto render(std::shared_ptr<geometry::Object>) -> void;
  auto render(std::shared_ptr<geometry::SkyBox>) -> void;
  auto render(std::shared_ptr<interface::Light>, std::shared_ptr<render::Shader>) -> void;

  auto push(glm::mat4 mat) -> void { stack_.push_back(mat); }
  auto pop() -> void { stack_.pop_back(); }

  auto lighting(bool val) -> void { lighting_ = val; }
  auto lighting() -> bool { return lighting_; }

  auto debugShader(std::shared_ptr<render::Shader> shader) -> void { debug_shader_ = shader; }
  auto debugTexture(std::shared_ptr<render::Texture> texture) -> void { debug_texture_ = texture; }
protected:
  void render(ObjectNodePtr node);

private:
  std::shared_ptr<render::Texture> debug_texture_;
  std::shared_ptr<render::Camera> camera_;
  std::shared_ptr<render::Shader> debug_shader_;
  std::vector<glm::mat4> stack_;
  bool lighting_{true};
};

}
}


