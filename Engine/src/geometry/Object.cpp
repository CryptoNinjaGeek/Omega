//
// Created by Carsten Tang on 13/08/2023.
//
#include "geometry/Object.h"
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Texture.h>

#include "glm/gtx/string_cast.hpp"
#include "glm/ext.hpp"

#include <reactphysics3d/reactphysics3d.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::geometry;

Object::Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
			   ObjectType type)
	: vao_(vao), vbo_(vbo), count_(cnt), type_(type), physicsObject_({.isActive = false}) {
  model_ = glm::mat4(1.0f);
}

Object::Object() : Entity() { model_ = glm::mat4(1.0f); }

void Object::render(std::shared_ptr<render::Camera> camera) {
  if (!visible_)
	return;

  if (!shader_) {
	std::cout << "No shader set for object: " << name_ << std::endl;
	return;
  }

  shader_->setMat4fv("projection", camera->projectionMatrix());
  shader_->setMat4fv("view", camera->viewMatrix());
  shader_->setMat4fv("model", model_);

  if (material_)
	shader_->setFloat("material.shininess", material_.value().shininess);

  for (int no = 0; no < textures_.size(); no++)
	textures_.at(no)->activate(no);

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
  case ObjectType::Array:glDrawArrays(GL_TRIANGLES, 0, count_);
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

auto Object::setupPhysics(reactphysics3d::PhysicsWorld *world,
						  reactphysics3d::PhysicsCommon *physicsCommon) -> void {
  if (!physicsObject_.isActive)
	return;
  reactphysics3d::Transform transform;
  transform.setFromOpenGL(glm::value_ptr(model_));

  body_ = world->createRigidBody(transform);
  body_->setType((reactphysics3d::BodyType)physicsObject_.bodyType);
  body_->setMass(physicsObject_.mass);

  reactphysics3d::CollisionShape *shape = nullptr;
  transform = reactphysics3d::Transform::identity();
  switch (physicsObject_.colliderType) {
  case physics::ColliderType::BOX:
	shape = physicsCommon
		->createBoxShape(reactphysics3d::Vector3(physicsObject_.boundingBox.x,
												 physicsObject_.boundingBox.y,
												 physicsObject_.boundingBox.z));
	break;
  case physics::ColliderType::PLANE:
	shape = physicsCommon
		->createBoxShape(reactphysics3d::Vector3(physicsObject_.boundingBox.x,
												 physicsObject_.boundingBox.y,
												 physicsObject_.boundingBox.z));
/*	transform.setPosition(reactphysics3d::Vector3(-1.f*physicsObject_.boundingBox.x,
												  -1.f*physicsObject_.boundingBox.y,
												  -1.f*physicsObject_.boundingBox.z));*/
	break;
  case physics::ColliderType::SPHERE: shape = physicsCommon->createSphereShape(1.0);
	break;
  }
  collider_ = body_->addCollider(shape, transform);

  reactphysics3d::Material &material = collider_->getMaterial();

  material.setBounciness(physicsObject_.bounciness);
  material.setFrictionCoefficient(physicsObject_.frictionCoefficient);
  material.setMassDensity(physicsObject_.massDensity);
}

auto Object::debug(bool val) -> void {
  body_->setIsDebugEnabled(val);
}

auto Object::process() -> void {
  if (body_) {
	auto transform = body_->getTransform();
	float mat[16];
	transform.getOpenGLMatrix(mat);
	model_ = glm::make_mat4(mat);
  }
}
