#include "physics/PhysicsEngine.h"
#include <physics/PhysicsObject.h>
#include <filesystem>
#include <iostream>

using namespace omega::physics;

static std::shared_ptr<PhysicsEngine> _manager;
unsigned int PhysicsEngine::id_counter_ = 0;

std::shared_ptr<PhysicsEngine> PhysicsEngine::instance() {
  if (!_manager) {
	_manager = std::make_shared<PhysicsEngine>();
  }
  return _manager;
}

auto PhysicsEngine::update(float deltaTime) -> void {
//  std::cout << "PhysicsEngine::update: " << deltaTime << std::endl;
}

auto PhysicsEngine::add(std::shared_ptr<PhysicsObject> obj) -> unsigned int {
  if (obj->id_!=0) {
	std::cout << "PhysicsEngine::add: Error => " << obj->id_ << " already added" << std::endl;
	return 0;
  }
  obj->id_ = id_counter_++;
  objects_.push_back(obj);
  return obj->id_;
}