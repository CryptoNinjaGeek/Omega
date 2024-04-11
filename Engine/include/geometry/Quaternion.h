#pragma once

#include <system/Global.h>

namespace omega {
namespace geometry {
template<class T>
class Point3;
typedef Point3<float> Vector;

class OMEGA_EXPORT Quaternion {
public:
  float x, y, z, w;

public:
  Quaternion(float Angle, const Vector &V);
  Quaternion(float _x, float _y, float _z, float _w);
  void normalize();
  Quaternion conjugate() const;
  Vector toDegrees();

  Quaternion operator*(const Quaternion &r);
  Quaternion operator*(const Vector &v);
};
}
}


