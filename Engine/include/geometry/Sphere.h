#pragma once

#include <geometry/Math.h>
#include <glm/gtx/norm.hpp>

namespace omega {
namespace geometry {
class Sphere {
public:
  glm::vec3 center;
  float radius;

public:
  Sphere() {}

  Sphere(const glm::vec3 &in_rPosition, const float in_rRadius)
	  : center(in_rPosition),
		radius(in_rRadius) {
	if (radius < 0.0f)
	  radius = 0.0f;
  }

  bool isContained(const glm::vec3 &in_rContain) const;

  bool isContained(const Sphere &in_rContain) const;

  bool isIntersecting(const Sphere &in_rIntersect) const;

  bool intersectsRay(const glm::vec3 &start, const glm::vec3 &end) const;

  float distanceTo(const glm::vec3 &pt) const;

  float squareDistanceTo(const glm::vec3 &pt) const;
};

inline bool Sphere::isContained(const glm::vec3 &in_rContain) const {
  float distSq = glm::length2(center - in_rContain);

  return (distSq <= (radius*radius));
}

inline bool Sphere::isContained(const Sphere &in_rContain) const {
  if (radius < in_rContain.radius)
	return false;

  // Since our radius is guaranteed to be >= other's, we
  //  can dodge the sqrt() here.
  //
  float dist = glm::length2(in_rContain.center - center);

  return (dist <= ((radius - in_rContain.radius)*
	  (radius - in_rContain.radius)));
}

inline bool Sphere::isIntersecting(const Sphere &in_rIntersect) const {
  float distSq = glm::length2(in_rIntersect.center - center);

  return (distSq <= ((in_rIntersect.radius + radius)*
	  (in_rIntersect.radius + radius)));
}

inline float Sphere::distanceTo(const glm::vec3 &toPt) const {
  return glm::length(center - toPt) - radius;
}
}
}
