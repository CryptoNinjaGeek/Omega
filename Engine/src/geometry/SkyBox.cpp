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
    : Object(vao, vbo, cnt) {}

void SkyBox::render(std::shared_ptr<render::Camera> camera) {
  auto view = glm::mat4(glm::mat3(
      camera->viewMatrix()));  // remove translation from the view matrix

  shader_->setMat4fv("projection", camera->projectionMatrix());
  shader_->setMat4fv("view", view);

  if (textures_.size()) textures_.at(0)->activate(0);

  glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when
                           // values are equal to depth buffer's content
  shader_->use();

  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, count_);
  glDepthFunc(GL_LESS);  // set depth function back to default
}
