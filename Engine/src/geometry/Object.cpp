//
// Created by Carsten Tang on 13/08/2023.
//
#include "geometry/Object.h"
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <system/Log.h>

#include "glm/gtx/string_cast.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <reactphysics3d/reactphysics3d.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::geometry;

Object::Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
               ObjectType type)
    : vao_(vao),
      vbo_(vbo),
      count_(cnt),
      type_(type),
      physicsObject_({.isActive = false}) {
  model_ = glm::mat4(1.0f);
}

Object::Object() : Entity() { model_ = glm::mat4(1.0f); }

void Object::render(std::shared_ptr<render::Camera> camera) {
  if (!visible_) return;

  if (!shader_) {
    OMEGA_LOG_WARN("object", "No shader set for object: {}", name_);
    return;
  }

  shader_->setMat4fv("projection", camera->projectionMatrix());
  shader_->setMat4fv("view", camera->viewMatrix());
  shader_->setMat4fv("model", model_);

  // Set viewPos for lighting calculations
  shader_->setVec3("viewPos", camera->position());

  // Always set material uniforms (required by shader)
  if (material_) {
    const auto& mat = material_.value();
    shader_->setFloat("material.shininess", mat.shininess);
    // If color alpha is > 0.5, shader will use color.rgb, otherwise
    // diffuseColor
    shader_->setVec4("material.color", mat.color.x, mat.color.y, mat.color.z,
                     mat.color.w);
    shader_->setVec3("material.diffuseColor", mat.diffuse.x, mat.diffuse.y,
                     mat.diffuse.z);
    shader_->setFloat("material.opacity", mat.opacity);
  } else {
    // Set defaults if no material (white color, full opacity, no tint)
    // Set color alpha to 0.0 so shader uses diffuseColor (white = no tint)
    shader_->setVec4("material.color", 1.0f, 1.0f, 1.0f, 0.0f);
    shader_->setVec3("material.diffuseColor", 1.0f, 1.0f,
                     1.0f);  // white = no tint
    shader_->setFloat("material.opacity", 1.0f);
    shader_->setFloat("material.shininess", 32.0f);  // Default shininess
  }

  for (int no = 0; no < textures_.size(); no++) textures_.at(no)->activate(no);

  shader_->resetCounters();
  shader_->turnOffLights();
  for (auto light : lights_) {
    light->setup(shader_);
  }

  shader_->use();

  glBindVertexArray(vao_);
  switch (type_) {
    case ObjectType::Elements:
      glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(count_),
                     GL_UNSIGNED_INT, 0);
      break;
    case ObjectType::Array:
      glDrawArrays(GL_TRIANGLES, 0, count_);
      break;
  }
  glBindVertexArray(0);
}

auto Object::position(glm::vec3 pos) -> void {
  model_ = glm::translate(model_, pos);
}

auto Object::scale(float value) -> void {
  model_ = glm::scale(model_, glm::vec3(value));
}

auto Object::setupPhysics(reactphysics3d::PhysicsWorld* world,
                          reactphysics3d::PhysicsCommon* physicsCommon)
    -> void {
  if (!physicsObject_.isActive) return;
  
  // Decompose model matrix to extract position, rotation, and scale separately
  // This is necessary because setFromOpenGL can fail with non-uniform scale
  glm::vec3 decomposedScale;
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(model_, decomposedScale, rotation, translation, skew, perspective);
  
  // Normalize rotation quaternion to ensure it's valid
  rotation = glm::normalize(rotation);
  
  // Check for invalid quaternion (NaN or Inf) - use identity if invalid
  if (std::isnan(rotation.w) || std::isnan(rotation.x) || std::isnan(rotation.y) || std::isnan(rotation.z) ||
      std::isinf(rotation.w) || std::isinf(rotation.x) || std::isinf(rotation.y) || std::isinf(rotation.z) ||
      glm::length(rotation) < 0.001f) { // Also check for zero-length quaternion
    OMEGA_LOG_WARN("object",
                   "Invalid rotation quaternion for object: {}, using identity",
                   name_);
    rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  }
  
  // Check translation for NaN/Inf - use model matrix position as fallback
  if (std::isnan(translation.x) || std::isnan(translation.y) || std::isnan(translation.z) ||
      std::isinf(translation.x) || std::isinf(translation.y) || std::isinf(translation.z)) {
    OMEGA_LOG_WARN("object",
                   "Invalid translation for object: {}, using model matrix position",
                   name_);
    translation = glm::vec3(model_[3][0], model_[3][1], model_[3][2]);
  }
  
  // Check decomposed scale for validity
  if (std::isnan(decomposedScale.x) || std::isnan(decomposedScale.y) || std::isnan(decomposedScale.z) ||
      std::isinf(decomposedScale.x) || std::isinf(decomposedScale.y) || std::isinf(decomposedScale.z) ||
      decomposedScale.x <= 0.0f || decomposedScale.y <= 0.0f || decomposedScale.z <= 0.0f) {
    OMEGA_LOG_WARN("object",
                   "Invalid decomposed scale for object: {}, using boundingBox scale",
                   name_);
    decomposedScale = glm::vec3(1.0f, 1.0f, 1.0f);
  }
  
  // Create transform from position and rotation (scale is handled by collider size)
  reactphysics3d::Transform transform;
  transform.setPosition(reactphysics3d::Vector3(translation.x, translation.y, translation.z));
  transform.setOrientation(reactphysics3d::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w));

  body_ = world->createRigidBody(transform);
  body_->setType((reactphysics3d::BodyType)physicsObject_.bodyType);
  body_->setMass(physicsObject_.mass);

  reactphysics3d::CollisionShape* shape = nullptr;
  transform = reactphysics3d::Transform::identity();
  switch (physicsObject_.colliderType) {
    case physics::ColliderType::BOX:
      shape = physicsCommon->createBoxShape(reactphysics3d::Vector3(
          physicsObject_.boundingBox.x, physicsObject_.boundingBox.y,
          physicsObject_.boundingBox.z));
      break;
    case physics::ColliderType::PLANE:
      shape = physicsCommon->createBoxShape(reactphysics3d::Vector3(
          physicsObject_.boundingBox.x, physicsObject_.boundingBox.y,
          physicsObject_.boundingBox.z));
      /*	transform.setPosition(reactphysics3d::Vector3(-1.f*physicsObject_.boundingBox.x,
                                                                                                        -1.f*physicsObject_.boundingBox.y,
                                                                                                        -1.f*physicsObject_.boundingBox.z));*/
      break;
    case physics::ColliderType::SPHERE:
      shape = physicsCommon->createSphereShape(1.0);
      break;
  }
  collider_ = body_->addCollider(shape, transform);

  reactphysics3d::Material& material = collider_->getMaterial();

  material.setBounciness(physicsObject_.bounciness);
  material.setFrictionCoefficient(physicsObject_.frictionCoefficient);
  material.setMassDensity(physicsObject_.massDensity);
}

auto Object::debug(bool val) -> void {
  if (body_) {
    body_->setIsDebugEnabled(val);
  }
}

auto Object::process() -> void {
  if (body_) {
    auto transform = body_->getTransform();
    float mat[16];
    transform.getOpenGLMatrix(mat);
    model_ = glm::make_mat4(mat);
  }
}
