#pragma once

#include <geometry/Math.h>
#include <geometry/BoundingBox.h>
#include <glm/glm.hpp>

namespace omega {
namespace geometry {

enum class PlaneSide {
  Front = 1,
  On = 0,
  Back = -1
};

class Plane {
public:
  glm::vec4 data;

  Plane();

  Plane(const BoundingBox &p, const glm::vec3 &n);

  Plane(float _x, float _y, float _z, float _d);

  Plane(const glm::vec3 &j, const glm::vec3 &k, const glm::vec3 &l);

  Plane(const glm::vec3 &p, const glm::vec3 &n);

  void set(const float _x, const float _y, const float _z);

  void set(const glm::vec3 &p, const glm::vec3 &n);

  void set(const glm::vec3 &k, const glm::vec3 &j, const glm::vec3 &l);

  void setPoint(const glm::vec3 &p);

  void setXY(float zz);

  void setYZ(float xx);

  void setXZ(float yy);

  void setXY(const glm::vec3 &P, float dir);

  void setYZ(const glm::vec3 &P, float dir);

  void setXZ(const glm::vec3 &P, float dir);

  void shiftX(float xx);

  void shiftY(float yy);

  void shiftZ(float zz);

  void invert();

  void neg();

  glm::vec3 project(const glm::vec3 &pt) const;

  float distTPlane(const glm::vec3 &cp) const;

  PlaneSide whichSide(const glm::vec3 &cp) const;

  float intersect(const glm::vec3 &start, const glm::vec3 &end) const;

  bool isHorizontal() const;

  bool isVertical() const;

  PlaneSide whichSideBox(const glm::vec3 &center,
						 const glm::vec3 &axisx,
						 const glm::vec3 &axisy,
						 const glm::vec3 &axisz) const;
};

inline Plane::Plane() {
}

inline Plane::Plane(float _x, float _y, float _z, float _d) {
  data = glm::vec4(_x, _y, _z, _d);
}

inline Plane::Plane(const glm::vec3 &p, const glm::vec3 &n) {
  set(p, n);
}

inline Plane::Plane(const glm::vec3 &j, const glm::vec3 &k, const glm::vec3 &l) {
  set(j, k, l);
}

inline void Plane::setXY(float zz) {
  data = glm::vec4(0, 0, 1, -zz);
}

inline void Plane::setYZ(float xx) {
  data = glm::vec4(1, 0, 0, -xx);
}

inline void Plane::setXZ(float yy) {
  data = glm::vec4(0, 1, 0, -yy);
}

inline void Plane::setXY(const glm::vec3 &point, float dir)       // Normal = (0, 0, -1|1)
{
  data = glm::vec4(0, 0, dir, -dir*point.z);
}

inline void Plane::setYZ(const glm::vec3 &point, float dir)       // Normal = (-1|1, 0, 0)
{
  data = glm::vec4(dir, 0, 0, -dir*point.x);
}

inline void Plane::setXZ(const glm::vec3 &point, float dir)       // Normal = (0, -1|1, 0)
{
  data = glm::vec4(0, dir, 0, -dir*point.y);
}

inline void Plane::shiftX(float xx) {
  data.w -= xx*data.x;
}

inline void Plane::shiftY(float yy) {
  data.w -= yy*data.y;
}

inline void Plane::shiftZ(float zz) {
  data.w -= zz*data.z;
}

inline bool Plane::isHorizontal() const {
  return (data.x==0 && data.y==0) ? true : false;
}

inline bool Plane::isVertical() const {
  return ((data.x!=0 || data.y!=0) && data.z==0) ? true : false;
}

inline glm::vec3 Plane::project(const glm::vec3 &pt) const {
  float dist = distTPlane(pt);
  return glm::vec3(pt.x - data.x*dist, pt.y - data.y*dist, pt.z - data.z*dist);
}

inline float Plane::distTPlane(const glm::vec3 &cp) const {
  return (data.x*cp.x + data.y*cp.y + data.z*cp.z) + data.w;
}

inline PlaneSide Plane::whichSide(const glm::vec3 &cp) const {
  float dist = distTPlane(cp);
  if (dist >= 0.005f)                 // if (mFabs(dist) < 0.005f)
	return PlaneSide::Front;                    //    return On;
  else if (dist <= -0.005f)           // else if (dist > 0.0f)
	return PlaneSide::Back;                     //    return front;
  else                                // else
	return PlaneSide::On;                       //    return Back;
}

inline void Plane::set(const float _x, const float _y, const float _z) {
  data = glm::vec4(_x, _y, _z, 0.f);
}

inline void Plane::setPoint(const glm::vec3 &p) {
  data.w = -(p.x*data.x + p.y*data.y + p.z*data.z);
}

inline void Plane::set(const glm::vec3 &p, const glm::vec3 &n) {
  data.x = n.x;
  data.y = n.y;
  data.z = n.z;

  data = glm::normalize(data);

  data.w = -(p.x*data.x + p.y*data.y + p.z*data.z);
}

inline void Plane::set(const glm::vec3 &k, const glm::vec3 &j, const glm::vec3 &l) {
  float ax = k.x - j.x;
  float ay = k.y - j.y;
  float az = k.z - j.z;
  float bx = l.x - j.x;
  float by = l.y - j.y;
  float bz = l.z - j.z;

  data.x = ay*bz - az*by;
  data.y = az*bx - ax*bz;
  data.z = ax*by - ay*bx;

  float squared = data.x*data.x + data.y*data.y + data.z*data.z;

  // In non-debug mode
  if (squared!=0) {
	float invSqrt = 1.0/(float)glm::distance2(data, data);
	data.x *= invSqrt;
	data.y *= invSqrt;
	data.z *= invSqrt;
	data.w = -(j.x*data.x + j.y*data.y + j.z*data.z);
  } else {
	data.x = 0;
	data.y = 0;
	data.z = 1;
	data.w = -(j.x*data.x + j.y*data.y + j.z*data.z);
  }
}

inline void Plane::invert() {
  data = glm::vec4(-data.x, -data.y, -data.z, -data.w);
}

inline void Plane::neg() {
  invert();
}

inline float Plane::intersect(const glm::vec3 &p1, const glm::vec3 &p2) const {
  float den = glm::dot(p2 - p1, glm::vec3(data));
  if (den==0)
	return PARALLEL_PLANE;
  return -distTPlane(p1)/den;
}

inline PlaneSide Plane::whichSideBox(const glm::vec3 &center,
									 const glm::vec3 &axisx,
									 const glm::vec3 &axisy,
									 const glm::vec3 &axisz) const {
  float baseDist = distTPlane(center);

  float compDist = mFabs(glm::dot(axisx, glm::vec3(data))) +
	  mFabs(glm::dot(axisy, glm::vec3(data))) +
	  mFabs(glm::dot(axisz, glm::vec3(data)));

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