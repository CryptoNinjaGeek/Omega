//
// Created by Carsten Tang on 13/08/2023.
//
#include "geometry/SkyBox.h"
#include <render/Camera.h>
#include <render/Texture.h>

#include "glm/gtx/string_cast.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::geometry;

SkyBox::SkyBox(unsigned int vao, unsigned int vbo, unsigned int cnt)
	: Object(vao, vbo, cnt) {
  data_->type_ = ObjectType::SkyBox;
}