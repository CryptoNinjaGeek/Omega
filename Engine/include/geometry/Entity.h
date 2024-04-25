#pragma once

#include "system/Global.h"
#include "render/Material.h"
#include <memory>
#include <vector>
#include <iostream>

#include "physics/PhysicsObjectInput.h"
#include "physics/PhysicsObject.h"

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace interface {

class OMEGA_EXPORT Entity {
public:
  Entity() = default;

  virtual auto entityModel() -> glm::mat4 { return modelMatrix_; }
  virtual auto entityPosition() -> glm::vec3 { return glm::vec3(0.0f); }
  virtual auto entityDirection() -> glm::vec3 { return glm::vec3(0.0f); }

  void physics(physics::PhysicsObjectInput object);

  auto follow(std::shared_ptr<Entity> entity) -> void {
	follow_ = entity;
	_follow_difference = entity->entityPosition() - entityPosition();
  }
  auto follow() -> std::shared_ptr<Entity> { return follow_; }
  auto lighting(bool val) -> void { lightingEnabled_ = val; }

protected:
  bool lightingEnabled_{true};
  glm::mat4 modelMatrix_;
  std::shared_ptr<Entity> follow_;
  glm::vec3 _follow_difference;
  std::shared_ptr<physics::PhysicsObject> physicsObject_;
};
}  // namespace interface
}  // namespace omega
