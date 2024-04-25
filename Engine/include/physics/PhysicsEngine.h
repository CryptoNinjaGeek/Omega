#pragma once

#include "system/Global.h"
#include <memory>
#include <map>

namespace omega {
namespace physics {

class PhysicsObject;

class OMEGA_EXPORT PhysicsEngine {
public:
  PhysicsEngine() = default;

  auto gravity(glm::vec3 value) -> void { gravity_ = value; }
  auto gravity() -> glm::vec3 { return gravity_; }

  auto verbose(bool value) -> void { verbose_ = value; }
  auto verbose() -> bool { return verbose_; }

  auto enabled(bool value) -> void { enabled_ = value; }
  auto enabled() -> bool { return enabled_; }

  auto update(float) -> void;

  auto getNbBodies() -> unsigned int { return objects_.size(); }
  auto body(unsigned int index) -> std::shared_ptr<PhysicsObject> { return objects_[index]; }
  auto add(std::shared_ptr<PhysicsObject> obj) -> unsigned int;

  static std::shared_ptr<PhysicsEngine> instance();
private:
  std::vector<std::shared_ptr<PhysicsObject>> objects_;
  bool verbose_{false};
  bool enabled_{false};
  glm::vec3 gravity_{0.0f, -9.81f, 0.0f};

  static unsigned int id_counter_;
};

}  // namespace fs
}  // namespace omega
