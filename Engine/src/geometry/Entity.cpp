#include <geometry/Entity.h>
#include <physics/PhysicsEngine.h>

using namespace omega::interface;
using namespace omega::physics;

void Entity::physics(physics::PhysicsObjectInput input) {
  physicsObject_ = std::make_shared<physics::PhysicsObject>(input);

  PhysicsEngine::instance()->add(physicsObject_);
}
