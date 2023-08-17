#pragma once

#include <string>
#include <memory>
#include <system/Global.h>

namespace omega {
namespace render {
class Texture;

struct Material {
  float shininess{16.f};
  std::shared_ptr<Texture> specular;
};
};  // namespace render
};  // namespace omega
