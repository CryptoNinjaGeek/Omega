#include <render/CubeTexture.h>
#include <system/FileSystem.h>

#include <iostream>
#include <glad/glad.h>

using namespace omega::render;

bool CubeTexture::load(input::CubeTextureInput input) {
  std::vector<std::string> faces{input.right,  input.left,  input.top,
                                 input.bottom, input.front, input.back};
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

  for (unsigned int i = 0; i < faces.size(); i++) {
    auto imageInfo = loadImageData(faces[i], false);

    if (imageInfo.data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                   imageInfo.width, imageInfo.height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, imageInfo.data);
      free(imageInfo.data);
    } else {
      std::cout << "Cubemap texture failed to load at path: " << faces[i]
                << std::endl;
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return true;
}

bool CubeTexture::activate(int no) {
  // bind textures on corresponding texture units
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);
}
