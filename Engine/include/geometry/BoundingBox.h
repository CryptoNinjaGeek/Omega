#pragma once

#include <geometry/Math.h>
#include <geometry/Sphere.h>
#include <geometry/BoxBase.h>

namespace omega {
namespace geometry {
class BoundingBox : public BoxBase {
public:
  glm::vec3 minExtents; ///< Minimum extents of box
  glm::vec3 maxExtents; ///< Maximum extents of box

  BoundingBox() {}
  BoundingBox(const glm::vec3 &in_rMin, const glm::vec3 &in_rMax);

  BoundingBox(const float &xMin, const float &yMin, const float &zMin,
			  const float &xMax, const float &yMax, const float &zMax);

  BoundingBox(float cubeSize);

  void set(const glm::vec3 &in_rMin, const glm::vec3 &in_rMax);

  void set(const float &xMin, const float &yMin, const float &zMin,
		   const float &xMax, const float &yMax, const float &zMax);

  /// Create box around origin given lengths
  void set(const glm::vec3 &in_Length);

  /// Recenter the box
  void setCenter(const glm::vec3 &center);

  /// Check to see if a point is contained in this box.
  bool isContained(const glm::vec3 &in_rContained) const;

  /// Check if the Point2<T> is within the box xy extents.
  bool isContained(const glm::vec2 &pt) const;

  /// Check to see if another box overlaps this box.
  bool isOverlapped(const BoundingBox &in_rOverlap) const;

  /// Check if the given sphere overlaps with the box.
  bool isOverlapped(const Sphere &sphere) const;

  /// Check to see if another box is contained in this box.
  bool isContained(const BoundingBox &in_rContained) const;

  /// Returns the length of the x extent.
  float len_x() const { return maxExtents.x - minExtents.x; }

  /// Returns the length of the y extent.
  float len_y() const { return maxExtents.y - minExtents.y; }

  /// Returns the length of the z extent.
  float len_z() const { return maxExtents.z - minExtents.z; }

  /// Returns the minimum box extent.
  float len_min() const { return fmin(len_x(), fmin(len_y(), len_z())); }

  /// Returns the maximum box extent.
  float len_max() const { return fmax(len_x(), fmax(len_y(), len_z())); }

  /// Returns the diagonal box length.
  float len() const { return glm::length(maxExtents - minExtents); }

  /// Returns the length of extent by axis index.
  ///
  /// @param axis The axis index of 0, 1, or 2.
  ///
  float len(int axis) const { return maxExtents[axis] - minExtents[axis]; }

  /// Returns true if any of the extent axes
  /// are less than or equal to zero.
  bool isEmpty() const { return len_x() <= 0.0f || len_y() <= 0.0f || len_z() <= 0.0f; }

  /// Perform an intersection operation with another box
  /// and store the results in this box.
  void intersect(const BoundingBox &in_rIntersect);

  void intersect(const glm::vec3 &in_rIntersect);

  /// Return the overlap between this box and @a otherBox.
  BoundingBox getOverlap(const BoundingBox &otherBox) const;

  /// Return the volume of the box.
  float getVolume() const;

  /// Get the center of this box.
  ///
  /// This is the average of min and max.
  void getCenter(glm::vec3 *center) const;

  glm::vec3 center() const;

  /// Returns the max minus the min extents.
  glm::vec3 getExtents() const { return maxExtents - minExtents; }

  glm::vec3 min() const { return minExtents; }
  glm::vec3 max() const { return maxExtents; }

  /// Collide a line against the box.
  ///
  /// @param   start   Start of line.
  /// @param   end     End of line.
  /// @param   float       Value from 0.0-1.0, indicating position
  ///                  along line of collision.
  /// @param   n       Normal of collision.
  bool collideLine(const glm::vec3 &start, const glm::vec3 &end, float *t, glm::vec3 *n) const;

  /// Collide a line against the box.
  ///
  /// Returns true on collision.
  bool collideLine(const glm::vec3 &start, const glm::vec3 &end) const;

  /// Collide an oriented box against the box.
  ///
  /// Returns true if "oriented" box collides with us.
  /// Assumes incoming box is centered at origin of source space.
  ///
  /// @param   radii   The dimension of incoming box (half x,y,z length).
  /// @param   toUs    A transform that takes incoming box into our space.
//            bool collideOrientedBox(const Point3 <T> &radii, const OMatrix<T> &toUs) const;

  /// Check that the min extents of the box is
  /// less than or equal to the max extents.
  bool isValidBox() const {
	return (minExtents.x <= maxExtents.x) &&
		(minExtents.y <= maxExtents.y) &&
		(minExtents.z <= maxExtents.z);
  }

  /// Return the closest point of the box, relative to the passed point.
  glm::vec3 getClosestPoint(const glm::vec3 &refPt) const;

  /// Return distance of closest point on box to refPt.
  float getDistanceTPoint(const glm::vec3 &refPt) const;

  /// Return the squared distance to  closest point on box to refPt.
  float getSqDistanceTPoint(const glm::vec3 &refPt) const;

  /// Return one of the corner vertices of the box.
  glm::vec3 computeVertex(unsigned int corner) const;

  /// Get the length of the longest diagonal in the box.
  float getGreatestDiagonalLength() const;

  /// Return the bounding sphere that contains this AABB.
  Sphere getBoundingSphere() const;

  /// Extend the box to include point.
  /// @see Invalid
  void extend(const glm::vec3 &p);

  /// Scale the box by a glm::vec3 or T
  void scale(const glm::vec3 &amt);

  void scale(float amt);

  /// Equality operator.
  bool operator==(const BoundingBox &b) const;

  /// Inequality operator.
  bool operator!=(const BoundingBox &b) const;

  /// Create an AABB that fits around the given point cloud, i.e.
  /// find the minimum and maximum extents of the given point set.
  static BoundingBox aroundPoints(const glm::vec3 *points, unsigned int numPoints);

public:

  /// An inverted bounds where the minimum point is positive
  /// and the maximum point is negative.  Should be used with
  /// extend() to construct a minimum volume box.
  /// @see extend
  static const BoundingBox Invalid;

  /// A box that covers the entire floating point range.
  static const BoundingBox Max;

  /// A null box of zero size.
  static const BoundingBox Zero;
};

inline BoundingBox::BoundingBox(const glm::vec3 &in_rMin, const glm::vec3 &in_rMax)
	: minExtents(in_rMin),
	  maxExtents(in_rMax) {
}

inline BoundingBox::BoundingBox(const float &xMin, const float &yMin, const float &zMin,
								const float &xMax, const float &yMax, const float &zMax)
	: minExtents(xMin, yMin, zMin),
	  maxExtents(xMax, yMax, zMax) {
}

inline BoundingBox::BoundingBox(float cubeSize)
	: minExtents(-0.5f*cubeSize, -0.5f*cubeSize, -0.5f*cubeSize),
	  maxExtents(0.5f*cubeSize, 0.5f*cubeSize, 0.5f*cubeSize) {
}

inline void BoundingBox::set(const glm::vec3 &in_rMin, const glm::vec3 &in_rMax) {
  minExtents = in_rMin;
  maxExtents = in_rMax;
}

inline void BoundingBox::set(const float &xMin, const float &yMin, const float &zMin,
							 const float &xMax, const float &yMax, const float &zMax) {
  minExtents = glm::vec3(xMin, yMin, zMin);
  maxExtents = glm::vec3(xMax, yMax, zMax);
}

inline void BoundingBox::set(const glm::vec3 &in_Length) {
  minExtents = glm::vec3(-in_Length.x*0.5f, -in_Length.y*0.5f, -in_Length.z*0.5f);
  maxExtents = glm::vec3(in_Length.x*0.5f, in_Length.y*0.5f, in_Length.z*0.5f);
}

inline void BoundingBox::setCenter(const glm::vec3 &center) {
  float halflenx = len_x()*0.5f;
  float halfleny = len_y()*0.5f;
  float halflenz = len_z()*0.5f;

  minExtents = glm::vec3(center.x - halflenx, center.y - halfleny, center.z - halflenz);
  maxExtents = glm::vec3(center.x + halflenx, center.y + halfleny, center.z + halflenz);
}

inline bool BoundingBox::isContained(const glm::vec3 &in_rContained) const {
  return (in_rContained.x >= minExtents.x && in_rContained.x < maxExtents.x) &&
	  (in_rContained.y >= minExtents.y && in_rContained.y < maxExtents.y) &&
	  (in_rContained.z >= minExtents.z && in_rContained.z < maxExtents.z);
}

inline bool BoundingBox::isContained(const glm::vec2 &pt) const {
  return (pt.x >= minExtents.x && pt.x < maxExtents.x) &&
	  (pt.y >= minExtents.y && pt.y < maxExtents.y);
}

inline bool BoundingBox::isOverlapped(const BoundingBox &in_rOverlap) const {
  if (in_rOverlap.minExtents.x > maxExtents.x ||
	  in_rOverlap.minExtents.y > maxExtents.y ||
	  in_rOverlap.minExtents.z > maxExtents.z)
	return false;
  if (in_rOverlap.maxExtents.x < minExtents.x ||
	  in_rOverlap.maxExtents.y < minExtents.y ||
	  in_rOverlap.maxExtents.z < minExtents.z)
	return false;
  return true;
}

inline bool BoundingBox::isContained(const BoundingBox &in_rContained) const {
  return (minExtents.x <= in_rContained.minExtents.x) &&
	  (minExtents.y <= in_rContained.minExtents.y) &&
	  (minExtents.z <= in_rContained.minExtents.z) &&
	  (maxExtents.x >= in_rContained.maxExtents.x) &&
	  (maxExtents.y >= in_rContained.maxExtents.y) &&
	  (maxExtents.z >= in_rContained.maxExtents.z);
}

inline void BoundingBox::intersect(const BoundingBox &in_rIntersect) {
  minExtents = glm::vec3(in_rIntersect.minExtents);
  maxExtents = glm::vec3(in_rIntersect.maxExtents);
}

inline void BoundingBox::intersect(const glm::vec3 &in_rIntersect) {
  minExtents = glm::vec3(in_rIntersect);
  maxExtents = glm::vec3(in_rIntersect);
}

inline BoundingBox BoundingBox::getOverlap(const BoundingBox &otherBox) const {
  BoundingBox overlap;

  for (unsigned int i = 0; i < 3; ++i) {
	if (minExtents[i] > otherBox.maxExtents[i] || otherBox.minExtents[i] > maxExtents[i]) {
	  overlap.minExtents[i] = 0.f;
	  overlap.maxExtents[i] = 0.f;
	} else {
	  overlap.minExtents[i] = fmin(minExtents[i], otherBox.minExtents[i]);
	  overlap.maxExtents[i] = fmin(maxExtents[i], otherBox.maxExtents[i]);
	}
  }

  return overlap;
}

inline float BoundingBox::getVolume() const {
  return (maxExtents.x - minExtents.x)*(maxExtents.y - minExtents.y)*(maxExtents.z - minExtents.z);
}

inline void BoundingBox::getCenter(glm::vec3 *center) const {
  center->x = (minExtents.x + maxExtents.x)*0.5f;
  center->y = (minExtents.y + maxExtents.y)*0.5f;
  center->z = (minExtents.z + maxExtents.z)*0.5f;
}

inline glm::vec3 BoundingBox::center() const {
  glm::vec3 center;
  center.x = (minExtents.x + maxExtents.x)*0.5f;
  center.y = (minExtents.y + maxExtents.y)*0.5f;
  center.z = (minExtents.z + maxExtents.z)*0.5f;
  return center;
}

inline glm::vec3 BoundingBox::getClosestPoint(const glm::vec3 &refPt) const {
  glm::vec3 closest;
  if (refPt.x <= minExtents.x)
	closest.x = minExtents.x;
  else if (refPt.x > maxExtents.x)
	closest.x = maxExtents.x;
  else
	closest.x = refPt.x;

  if (refPt.y <= minExtents.y)
	closest.y = minExtents.y;
  else if (refPt.y > maxExtents.y)
	closest.y = maxExtents.y;
  else
	closest.y = refPt.y;

  if (refPt.z <= minExtents.z)
	closest.z = minExtents.z;
  else if (refPt.z > maxExtents.z)
	closest.z = maxExtents.z;
  else
	closest.z = refPt.z;

  return closest;
}

inline float BoundingBox::getDistanceTPoint(const glm::vec3 &refPt) const {
  return mSqrt(getSqDistanceTPoint(refPt));
}

inline float BoundingBox::getSqDistanceTPoint(const glm::vec3 &refPt) const {
  float sqDist = 0.0f;

  for (unsigned int i = 0; i < 3; i++) {
	const float v = refPt[i];
	if (v < minExtents[i]) {
	  auto temp = minExtents[i] - v;
	  sqDist += glm::dot(temp, temp);
	} else if (v > maxExtents[i]) {
	  auto temp = v - maxExtents[i];
	  sqDist += glm::dot(temp, temp);
	}
  }

  return sqDist;
}

inline void BoundingBox::extend(const glm::vec3 &p) {
#define EXTEND_AXIS(AXIS)    \
if (p.AXIS < minExtents.AXIS)       \
   minExtents.AXIS = p.AXIS;        \
if (p.AXIS > maxExtents.AXIS)  \
   maxExtents.AXIS = p.AXIS;

  EXTEND_AXIS(x)
  EXTEND_AXIS(y)
  EXTEND_AXIS(z)

#undef EXTEND_AXIS
}

inline void BoundingBox::scale(const glm::vec3 &amt) {
  minExtents *= amt;
  maxExtents *= amt;
}

inline void BoundingBox::scale(float amt) {
  minExtents *= amt;
  maxExtents *= amt;
}

inline bool BoundingBox::operator==(const BoundingBox &b) const {
  return minExtents==b.minExtents && maxExtents==b.maxExtents;
}

inline bool BoundingBox::operator!=(const BoundingBox &b) const {
  return minExtents!=b.minExtents || maxExtents!=b.maxExtents;
}
};
}