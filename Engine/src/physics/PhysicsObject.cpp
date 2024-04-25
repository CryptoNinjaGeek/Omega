#include <physics/PhysicsObject.h>
#include <iostream>

using namespace omega::physics;

auto PhysicsObject::aabb() -> omega::geometry::BoundingBox {
  return input_.boundingBox;
}

auto PhysicsObject::move(glm::vec3 force) -> void {
  matrix_ = glm::translate(matrix_, force);
  std::cout << "Move: " << glm::to_string(force) << " <=> " << glm::to_string(matrix_) << std::endl;
}

auto PhysicsObject::applyForce(glm::vec3 force) -> void {
  matrix_ = glm::translate(matrix_, force);
  std::cout << "Applying force: " << glm::to_string(force) << " <=> " << glm::to_string(matrix_) << std::endl;
}

auto PhysicsObject::velocity() -> glm::vec3 {
  return glm::vec3(0.0f);
}

auto PhysicsObject::worldCenter() -> glm::vec3 {
  return glm::vec3(matrix_[3]) + input_.boundingBox.center();
}

auto PhysicsObject::center() -> glm::vec3 {
  return input_.boundingBox.center();
}