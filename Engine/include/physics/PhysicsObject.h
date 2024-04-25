#pragma once

#include "system/Global.h"
#include <geometry/BoundingBox.h>
#include <physics/PhysicsObjectInput.h>

#include <memory>
#include <map>

namespace omega {
namespace physics {

class OMEGA_EXPORT PhysicsObject {
  friend class PhysicsEngine;
public:
  PhysicsObject(physics::PhysicsObjectInput input) : input_(input) {}

  auto worldCenter() -> glm::vec3;
  auto center() -> glm::vec3;

  auto position() -> glm::vec3 { return matrix_[3]; }
  auto position(glm::vec3 position) -> void { matrix_[3] = glm::vec4(position, 1.0f); }

  auto rotation() -> glm::quat { return glm::quat_cast(matrix_); }
  auto rotation(glm::quat rotation) -> void { matrix_ = glm::mat4_cast(rotation); }

  auto scale() -> glm::vec3 {
	return glm::vec3(glm::length(matrix_[0]),
					 glm::length(matrix_[1]),
					 glm::length(matrix_[2]));
  }
  auto scale(glm::vec3 scale) -> void {
	matrix_[0] = glm::vec4(glm::normalize(matrix_[0])*scale.x);
	matrix_[1] = glm::vec4(glm::normalize(matrix_[1])*scale.y);
	matrix_[2] = glm::vec4(glm::normalize(matrix_[2])*scale.z);
  }

  auto matrix() -> glm::mat4 { return matrix_; }
  auto matrix(glm::mat4 matrix) -> void { matrix_ = matrix; }

  auto aabb() -> omega::geometry::BoundingBox;

  auto move(glm::vec3 force) -> void;
  auto applyForce(glm::vec3 force) -> void;
  auto velocity() -> glm::vec3;

protected:
  physics::PhysicsObjectInput input_;
  bool verbose_{false};
  glm::mat4 matrix_{1.0f};
  unsigned int id_{0};
};

}  // namespace fs
}  // namespace omega
