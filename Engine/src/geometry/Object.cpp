//
// Created by Carsten Tang on 13/08/2023.
//
#include "geometry/Object.h"
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/Material.h>
#include <utils/utils.h>

#include "glm/gtx/string_cast.hpp"
#include "glm/ext.hpp"

#include "physics/PhysicsEngine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::geometry;

Object::Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
			   RenderType type) {
  data_->type_ = ObjectType::Object;
  data_->vao_ = vao;
  data_->vbo_ = vbo;
  data_->count_ = cnt;
  data_->render_type_ = type;
  data_->model_ = glm::mat4(1.0f);
  data_->material_ = render::Material{.shininess = 0.5f};
}

Object::Object() : ObjectInterface() { data_->model_ = glm::mat4(1.0f); }

auto Object::scale(float value) -> void {
  data_->model_ = glm::scale(data_->model_, glm::vec3(value));
}

auto Object::rotate(float angle, glm::vec3 axis) -> void {
  data_->model_ = glm::rotate(data_->model_, glm::radians(angle), axis);
}

auto Object::translate(glm::vec3 pos) -> void {
  data_->model_ = glm::translate(data_->model_, pos);
}

auto Object::process() -> void {
  if (physicsObject_) {
	data_->model_ = physicsObject_->matrix();
  }
}

auto Object::prepare(input::ObjectPreparation input) -> void {
  affectedByLights(input.lights);
}

void Object::setShader(std::shared_ptr<render::Shader> shader) {
  data_->shader_ = shader;
}
