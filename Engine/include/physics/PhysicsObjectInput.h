#pragma once

#include  "system/Global.h"
#include <geometry/BoundingBox.h>

namespace omega {
namespace physics {

enum class ColliderType {
  BOX,
  SPHERE,
  PLANE
};

enum class BodyType {
  STATIC,
  KINEMATIC,
  DYNAMIC
};

struct PhysicsObjectInput {
  BodyType bodyType{BodyType::STATIC};
  ColliderType colliderType{ColliderType::BOX};
  geometry::BoundingBox boundingBox;
  float mass{1.0};
  float bounciness{0.5};
  float frictionCoefficient{0.3};
  float massDensity{1.0};
  bool isActive{true};
};
}
}


