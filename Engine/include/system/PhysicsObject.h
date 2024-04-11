#pragma once

#include  <system/Global.h>
#include <reactphysics3d/reactphysics3d.h>

namespace omega {
namespace physics {

enum class ColliderType {
  BOX,
  SPHERE,
  PLANE
};

enum class BodyType {
  STATIC = (int)reactphysics3d::BodyType::STATIC,
  KINEMATIC = (int)reactphysics3d::BodyType::KINEMATIC,
  DYNAMIC = (int)reactphysics3d::BodyType::DYNAMIC
};

struct PhysicsObject {
  BodyType bodyType{BodyType::STATIC};
  ColliderType colliderType{ColliderType::BOX};
  glm::vec3 boundingBox{1.0f, 1.0f, 1.0f};
  float mass{1.0};
  float bounciness{0.5};
  float frictionCoefficient{0.3};
  float massDensity{1.0};
  bool isActive{true};
};
}
}


