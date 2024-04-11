#pragma once

#include <geometry/Point3.h>
#include <geometry/Math.h>

namespace omega {
namespace geometry {

enum class PlaneSide {
  Front = 1,
  On = 0,
  Back = -1
};

template<class T>
class Plane : public Point3<T> {
public:
  T x;
  T y;
  T z;
  T d;

  Plane();

  Plane(const Point3<T> &p, const Point3<T> &n);

  Plane(T _x, T _y, T _z, T _d);

  Plane(const Point3<T> &j, const Point3<T> &k, const Point3<T> &l);

  void set(const T _x, const T _y, const T _z);

  void set(const Point3<T> &p, const Point3<T> &n);

  void set(const Point3<T> &k, const Point3<T> &j, const Point3<T> &l);

  void setPoint(const Point3<T> &p);

  void setXY(T zz);

  void setYZ(T xx);

  void setXZ(T yy);

  void setXY(const Point3<T> &P, T dir);

  void setYZ(const Point3<T> &P, T dir);

  void setXZ(const Point3<T> &P, T dir);

  void shiftX(T xx);

  void shiftY(T yy);

  void shiftZ(T zz);

  void invert();

  void neg();

  Point3<T> project(const Point3<T> &pt) const;

  T distTPlane(const Point3<T> &cp) const;

  PlaneSide whichSide(const Point3<T> &cp) const;

  T intersect(const Point3<T> &start, const Point3<T> &end) const;

  bool isHorizontal() const;

  bool isVertical() const;

  PlaneSide whichSideBox(const Point3<T> &center,
						 const Point3<T> &axisx,
						 const Point3<T> &axisy,
						 const Point3<T> &axisz) const;
};

template<class T>
inline Plane<T>::Plane() {
}

template<class T>
inline Plane<T>::Plane(T _x, T _y, T _z, T _d) {
  x = _x;
  y = _y;
  z = _z;
  d = _d;
}

template<class T>
inline Plane<T>::Plane(const Point3<T> &p, const Point3<T> &n) {
  set(p, n);
}

template<class T>
inline Plane<T>::Plane(const Point3<T> &j, const Point3<T> &k, const Point3<T> &l) {
  set(j, k, l);
}

template<class T>
inline void Plane<T>::setXY(T zz) {
  x = y = 0;
  z = 1;
  d = -zz;
}

template<class T>
inline void Plane<T>::setYZ(T xx) {
  x = 1;
  z = y = 0;
  d = -xx;
}

template<class T>
inline void Plane<T>::setXZ(T yy) {
  x = z = 0;
  y = 1;
  d = -yy;
}

template<class T>
inline void Plane<T>::setXY(const Point3<T> &point, T dir)       // Normal = (0, 0, -1|1)
{
  x = y = 0;
  d = -((z = dir)*point.z);
}

template<class T>
inline void Plane<T>::setYZ(const Point3<T> &point, T dir)       // Normal = (-1|1, 0, 0)
{
  z = y = 0;
  d = -((x = dir)*point.x);
}

template<class T>
inline void Plane<T>::setXZ(const Point3<T> &point, T dir)       // Normal = (0, -1|1, 0)
{
  x = z = 0;
  d = -((y = dir)*point.y);
}

template<class T>
inline void Plane<T>::shiftX(T xx) {
  d -= xx*x;
}

template<class T>
inline void Plane<T>::shiftY(T yy) {
  d -= yy*y;
}

template<class T>
inline void Plane<T>::shiftZ(T zz) {
  d -= zz*z;
}

template<class T>
inline bool Plane<T>::isHorizontal() const {
  return (x==0 && y==0) ? true : false;
}

template<class T>
inline bool Plane<T>::isVertical() const {
  return ((x!=0 || y!=0) && z==0) ? true : false;
}

template<class T>
inline Point3<T> Plane<T>::project(const Point3<T> &pt) const {
  T dist = distTPlane(pt);
  return Point3<T>(pt.x - x*dist, pt.y - y*dist, pt.z - z*dist);
}

template<class T>
inline T Plane<T>::distTPlane(const Point3<T> &cp) const {
  return (x*cp.x + y*cp.y + z*cp.z) + d;
}

template<class T>
inline PlaneSide Plane<T>::whichSide(const Point3<T> &cp) const {
  T dist = distTPlane(cp);
  if (dist >= 0.005f)                 // if (mFabs(dist) < 0.005f)
	return PlaneSide::Front;                    //    return On;
  else if (dist <= -0.005f)           // else if (dist > 0.0f)
	return PlaneSide::Back;                     //    return front;
  else                                // else
	return PlaneSide::On;                       //    return Back;
}

template<class T>
inline void Plane<T>::set(const T _x, const T _y, const T _z) {
  Point3<T>::set(_x, _y, _z);
}

template<class T>
inline void Plane<T>::setPoint(const Point3<T> &p) {
  d = -(p.x*x + p.y*y + p.z*z);
}

template<class T>
inline void Plane<T>::set(const Point3<T> &p, const Point3<T> &n) {
  x = n.x;
  y = n.y;
  z = n.z;

  this->normalize();

  d = -(p.x*x + p.y*y + p.z*z);
}

template<class T>
inline void Plane<T>::set(const Point3<T> &k, const Point3<T> &j, const Point3<T> &l) {
  T ax = k.x - j.x;
  T ay = k.y - j.y;
  T az = k.z - j.z;
  T bx = l.x - j.x;
  T by = l.y - j.y;
  T bz = l.z - j.z;
  x = ay*bz - az*by;
  y = az*bx - ax*bz;
  z = ax*by - ay*bx;
  T squared = x*x + y*y + z*z;

  // In non-debug mode
  if (squared!=0) {
	T invSqrt = 1.0/(T)mSqrtD(x*x + y*y + z*z);
	x *= invSqrt;
	y *= invSqrt;
	z *= invSqrt;
	d = -(j.x*x + j.y*y + j.z*z);
  } else {
	x = 0;
	y = 0;
	z = 1;
	d = -(j.x*x + j.y*y + j.z*z);
  }
}

template<class T>
inline void Plane<T>::invert() {
  x = -x;
  y = -y;
  z = -z;
  d = -d;
}

template<class T>
inline void Plane<T>::neg() {
  invert();
}

template<class T>
inline T Plane<T>::intersect(const Point3<T> &p1, const Point3<T> &p2) const {
  T den = mDot(p2 - p1, *this);
  if (den==0)
	return PARALLEL_PLANE;
  return -distTPlane(p1)/den;
}

template<class T>
inline PlaneSide Plane<T>::whichSideBox(const Point3<T> &center,
										const Point3<T> &axisx,
										const Point3<T> &axisy,
										const Point3<T> &axisz) const {
  T baseDist = distTPlane(center);

  T compDist = mFabs(mDot(axisx, *this)) +
	  mFabs(mDot(axisy, *this)) +
	  mFabs(mDot(axisz, *this));

  // Intersects
  if (baseDist >= compDist)
	return PlaneSide::Front;
  else if (baseDist <= -compDist)
	return PlaneSide::Back;
  else
	return PlaneSide::On;
}
}
}