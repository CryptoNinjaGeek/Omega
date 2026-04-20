#pragma once

#include <memory>
#include <system/Global.h>
#include <glm/glm.hpp>

namespace omega {
namespace render {
class Texture;

struct Material {
  float shininess{16.f};
  std::shared_ptr<Texture> specular;
  
  // Color properties (from JSON)
  glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};  // RGBA color
  glm::vec3 diffuse{1.0f, 1.0f, 1.0f};     // RGB diffuse color
  float opacity{1.0f};                      // Opacity (0.0-1.0)
};
};  // namespace render
};  // namespace omega
