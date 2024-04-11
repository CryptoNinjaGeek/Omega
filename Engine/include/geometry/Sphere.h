#pragma once

#include <geometry/Math.h>
#include <geometry/Point3.h>

namespace omega {
namespace geometry {
template<class T>
class Sphere {
public:
  Point3 <T> center;
  T radius;

public:
  Sphere<T>() {}

  Sphere<T>(const Point3 <T> &in_rPosition, const T in_rRadius)
	  : center(in_rPosition),
		radius(in_rRadius) {
	if (radius < 0.0f)
	  radius = 0.0f;
  }

  bool isContained(const Point3 <T> &in_rContain) const;

  bool isContained(const Sphere<T> &in_rContain) const;

  bool isIntersecting(const Sphere<T> &in_rIntersect) const;

  bool intersectsRay(const Point3 <T> &start, const Point3 <T> &end) const;

  T distanceTo(const Point3 <T> &pt) const;

  T squareDistanceTo(const Point3 <T> &pt) const;
};

template<class T>
inline bool Sphere<T>::isContained(const Point3 <T> &in_rContain) const {
  T distSq = (center - in_rContain).lenSquared();

  return (distSq <= (radius*radius));
}

template<class T>
inline bool Sphere<T>::isContained(const Sphere<T> &in_rContain) const {
  if (radius < in_rContain.radius)
	return false;

  // Since our radius is guaranteed to be >= other's, we
  //  can dodge the sqrt() here.
  //
  T dist = (in_rContain.center - center).lenSquared();

  return (dist <= ((radius - in_rContain.radius)*
	  (radius - in_rContain.radius)));
}

template<class T>
inline bool Sphere<T>::isIntersecting(const Sphere<T> &in_rIntersect) const {
  T distSq = (in_rIntersect.center - center).lenSquared();

  return (distSq <= ((in_rIntersect.radius + radius)*
	  (in_rIntersect.radius + radius)));
}

template<class T>
inline T Sphere<T>::distanceTo(const Point3 <T> &toPt) const {
  return (center - toPt).len() - radius;
}
}
}
