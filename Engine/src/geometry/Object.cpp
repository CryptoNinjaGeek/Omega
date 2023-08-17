//
// Created by Carsten Tang on 13/08/2023.
//
#include "geometry/Object.h"
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Texture.h>

#include "glm/gtx/string_cast.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::geometry;

Object::Object(unsigned int vao, unsigned int vbo, unsigned int cnt)
    : vao_(vao), vbo_(vbo), count_(cnt) {
  model_ = glm::mat4(1.0f);
}

Object::Object() : Entity() { model_ = glm::mat4(1.0f); }

void Object::render(std::shared_ptr<render::Camera> camera) {
  shader_->setMat4fv("projection", camera->projectionMatrix());
  shader_->setMat4fv("view", camera->viewMatrix());
  shader_->setMat4fv("model", model_);

  if (material_)
    shader_->setFloat("material.shininess", material_.value().shininess);

  for (int no = 0; no < textures_.size(); no++) textures_.at(no)->activate(no);

  shader_->resetCounters();
  shader_->turnOffLights();
  for (auto light : lights_) {
    light->setup(shader_);
  }

  shader_->use();

  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, count_);
}

void Object::setPosition(glm::vec3 pos) {
  auto matrix = glm::mat4(1.0f);
  model_ = glm::translate(matrix, pos);
}
