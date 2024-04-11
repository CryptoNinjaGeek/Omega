#define STB_IMAGE_IMPLEMENTATION
#include <render/Texture.h>
#include <system/FileSystem.h>

#include <stb_image.h>
#include <iostream>
#include <glad/glad.h>
using namespace omega::render;

Texture::Texture() {}

auto Texture::loadImageData(std::string fileName, bool flip, std::string name) -> ImageInfo {
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(flip);
  auto bytes = fs::instance()->data(fileName);

  unsigned char *data = stbi_load_from_memory(bytes.data(), bytes.size(),
											  &width, &height, &nrChannels, 0);

  return {
	  .data = data, .width = width, .height = height, .channels = nrChannels};
}

bool Texture::load(std::string fileName, std::string name) {
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_2D, m_textureId);
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_set_flip_vertically_on_load(true);

  auto imageInfo = loadImageData(fileName);
  if (imageInfo.data) {
	int format{GL_RGB};
	if (imageInfo.channels==1)
	  format = GL_RED;
	else if (imageInfo.channels==3)
	  format = GL_RGB;
	else if (imageInfo.channels==4)
	  format = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, format, imageInfo.width, imageInfo.height, 0,
				 format, GL_UNSIGNED_BYTE, imageInfo.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	_name = name.empty() ? fileName : name;
  } else {
	std::cout << "Failed to load texture: " << fileName << std::endl;
  }
  free(imageInfo.data);
  return true;
}

bool Texture::activate(int no) {
  // bind textures on corresponding texture units
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(GL_TEXTURE_2D, m_textureId);
}
