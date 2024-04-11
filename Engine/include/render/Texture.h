#pragma once

#include <string>

#include <system/Global.h>

namespace omega {
namespace render {
struct ImageInfo {
  unsigned char *data;
  int width;
  int height;
  int channels;
};

class OMEGA_EXPORT Texture {
public:
  Texture();

  virtual auto activate(int no) -> bool;

  auto load(std::string fileName, std::string name = {}) -> bool;
  auto name() -> std::string { return _name; }
  auto name(std::string name) -> void { _name = name; }

protected:
  auto loadImageData(std::string fileName, bool flip = true, std::string name = {}) -> ImageInfo;

protected:
  unsigned int m_textureId{0};
  std::string _name;
};
};  // namespace render
};  // namespace omega
