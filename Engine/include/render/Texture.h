#pragma once

#include <string>

#include <system/Global.h>

namespace omega {
namespace render {
struct ImageInfo {
  unsigned char* data;
  int width;
  int height;
  int channels;
};

class OMEGA_EXPORT Texture {
 public:
  Texture();
  bool load(std::string fileName);
  virtual bool activate(int no);

 protected:
  auto loadImageData(std::string fileName, bool flip = true) -> ImageInfo;

 protected:
  unsigned int m_textureId{0};
};
};  // namespace render
};  // namespace omega
