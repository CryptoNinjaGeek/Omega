#pragma once

#include <string>

#include <system/Global.h>
#include <render/Texture.h>

namespace omega {
namespace input {
struct CubeTextureInput {
  std::string front;
  std::string back;
  std::string left;
  std::string right;
  std::string top;
  std::string bottom;
};
}  // namespace input
namespace render {

class OMEGA_EXPORT CubeTexture : public Texture {
public:
  CubeTexture() = default;
  bool load(omega::input::CubeTextureInput);
  virtual bool activate(int no);
};
};  // namespace render
};  // namespace omega
