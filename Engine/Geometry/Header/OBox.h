#pragma once

#include "OMath.h"
#include "OPoint3.h"
#include "OPoint2.h"
#include "OMatrix.h"
#include "OSphere.h"
#include "OBoxBase.h"

namespace omega {
    namespace geometry {
        template<class T>
        class OBox3 : public OBoxBase<T> {
        public:
            OPoint3 <T> minExtents; ///< Minimum extents of box
            OPoint3 <T> maxExtents; ///< Maximum extents of box

            OBox3() {}
            OBox3(const OPoint3 <T> &in_rMin, const OPoint3 <T> &in_rMax, const bool in_overrideCheck = false);

            OBox3(const T &xMin, const T &yMin, const T &zMin,
                  const T &xMax, const T &yMax, const T &zMax);

            OBox3(T cubeSize);

            void set(const OPoint3 <T> &in_rMin, const OPoint3 <T> &in_rMax);

            void set(const T &xMin, const T &yMin, const T &zMin,
                     const T &xMax, const T &yMax, const T &zMax);

            /// Create box around origin given lengths
            void set(const OPoint3 <T> &in_Length);

            /// Recenter the box
            void setCenter(const OPoint3 <T> &center);

            /// Check to see if a point is contained in this box.
            bool isContained(const OPoint3 <T> &in_rContained) const;

            /// Check if the OPoint2<T> is within the box xy extents.
            bool isContained(const OPoint2<T> &pt) const;

            /// Check to see if another box overlaps this box.
            bool isOverlapped(const OBox3 &in_rOverlap) const;

            /// Check if the given sphere overlaps with the box.
            bool isOverlapped(const OSphere<T> &sphere) const;

            /// Check to see if another box is contained in this box.
            bool isContained(const OBox3 &in_rContained) const;

            /// Returns the length of the x extent.
            T len_x() const { return maxExtents.x - minExtents.x; }

            /// Returns the length of the y extent.
            T len_y() const { return maxExtents.y - minExtents.y; }

            /// Returns the length of the z extent.
            T len_z() const { return maxExtents.z - minExtents.z; }

            /// Returns the minimum box extent.
            T len_min() const { return getMin(len_x(), getMin(len_y(), len_z())); }

            /// Returns the maximum box extent.
            T len_max() const { return getMax(len_x(), getMax(len_y(), len_z())); }

            /// Returns the diagonal box length.
            T len() const { return (maxExtents - minExtents).len(); }

            /// Returns the length of extent by axis index.
            ///
            /// @param axis The axis index of 0, 1, or 2.
            ///
            T len(int axis) const { return maxExtents[axis] - minExtents[axis]; }

            /// Returns true if any of the extent axes
            /// are less than or equal to zero.
            bool isEmpty() const { return len_x() <= 0.0f || len_y() <= 0.0f || len_z() <= 0.0f; }

            /// Perform an intersection operation with another box
            /// and store the results in this box.
            void intersect(const OBox3 &in_rIntersect);

            void intersect(const OPoint3 <T> &in_rIntersect);

            /// Return the overlap between this box and @a otherBox.
            OBox3 getOverlap(const OBox3 &otherBox) const;

            /// Return the volume of the box.
            T getVolume() const;

            /// Get the center of this box.
            ///
            /// This is the average of min and max.
            void getCenter(OPoint3 <T> *center) const;

            OPoint3 <T> getCenter() const;

            /// Returns the max minus the min extents.
            OPoint3 <T> getExtents() const { return maxExtents - minExtents; }

            /// Collide a line against the box.
            ///
            /// @param   start   Start of line.
            /// @param   end     End of line.
            /// @param   t       Value from 0.0-1.0, indicating position
            ///                  along line of collision.
            /// @param   n       Normal of collision.
            bool collideLine(const OPoint3 <T> &start, const OPoint3 <T> &end, T *t, OPoint3 <T> *n) const;

            /// Collide a line against the box.
            ///
            /// Returns true on collision.
            bool collideLine(const OPoint3 <T> &start, const OPoint3 <T> &end) const;

            /// Collide an oriented box against the box.
            ///
            /// Returns true if "oriented" box collides with us.
            /// Assumes incoming box is centered at origin of source space.
            ///
            /// @param   radii   The dimension of incoming box (half x,y,z length).
            /// @param   toUs    A transform that takes incoming box into our space.
//            bool collideOrientedBox(const OPoint3 <T> &radii, const OMatrix<T> &toUs) const;

            /// Check that the min extents of the box is
            /// less than or equal to the max extents.
            bool isValidBox() const {
                return (minExtents.x <= maxExtents.x) &&
                       (minExtents.y <= maxExtents.y) &&
                       (minExtents.z <= maxExtents.z);
            }

            /// Return the closest point of the box, relative to the passed point.
            OPoint3 <T> getClosestPoint(const OPoint3 <T> &refPt) const;

            /// Return distance of closest point on box to refPt.
            T getDistanceToPoint(const OPoint3 <T> &refPt) const;

            /// Return the squared distance to  closest point on box to refPt.
            T getSqDistanceToPoint(const OPoint3 <T> &refPt) const;

            /// Return one of the corner vertices of the box.
            OPoint3 <T> computeVertex(unsigned int corner) const;

            /// Get the length of the longest diagonal in the box.
            T getGreatestDiagonalLength() const;

            /// Return the bounding sphere that contains this AABB.
            OSphere<T> getBoundingSphere() const;

            /// Extend the box to include point.
            /// @see Invalid
            void extend(const OPoint3 <T> &p);

            /// Scale the box by a OPoint3<T> or T
            void scale(const OPoint3 <T> &amt);

            void scale(T amt);

            /// Equality operator.
            bool operator==(const OBox3 &b) const;

            /// Inequality operator.
            bool operator!=(const OBox3 &b) const;

            /// Create an AABB that fits around the given point cloud, i.e.
            /// find the minimum and maximum extents of the given point set.
            static OBox3 aroundPoints(const OPoint3 <T> *points, unsigned int numPoints);

        public:

            /// An inverted bounds where the minimum point is positive
            /// and the maximum point is negative.  Should be used with
            /// extend() to construct a minimum volume box.
            /// @see extend
            static const OBox3 Invalid;

            /// A box that covers the entire floating point range.
            static const OBox3 Max;

            /// A null box of zero size.
            static const OBox3 Zero;
        };

        template<class T>
        inline OBox3<T>::OBox3(const OPoint3 <T> &in_rMin, const OPoint3 <T> &in_rMax, const bool in_overrideCheck)
                : minExtents(in_rMin),
                  maxExtents(in_rMax) {
            if (in_overrideCheck == false) {
                minExtents.setMin(in_rMax);
                maxExtents.setMax(in_rMin);
            }
        }

        template<class T>
        inline OBox3<T>::OBox3(const T &xMin, const T &yMin, const T &zMin,
                            const T &xMax, const T &yMax, const T &zMax)
                : minExtents(xMin, yMin, zMin),
                  maxExtents(xMax, yMax, zMax) {
        }

        template<class T>
        inline OBox3<T>::OBox3(T cubeSize)
                : minExtents(-0.5f * cubeSize, -0.5f * cubeSize, -0.5f * cubeSize),
                  maxExtents(0.5f * cubeSize, 0.5f * cubeSize, 0.5f * cubeSize) {
        }

        template<class T>
        inline void OBox3<T>::set(const OPoint3 <T> &in_rMin, const OPoint3 <T> &in_rMax) {
            minExtents.set(in_rMin);
            maxExtents.set(in_rMax);
        }

        template<class T>
        inline void OBox3<T>::set(const T &xMin, const T &yMin, const T &zMin,
                               const T &xMax, const T &yMax, const T &zMax) {
            minExtents.set(xMin, yMin, zMin);
            maxExtents.set(xMax, yMax, zMax);
        }

        template<class T>
        inline void OBox3<T>::set(const OPoint3 <T> &in_Length) {
            minExtents.set(-in_Length.x * 0.5f, -in_Length.y * 0.5f, -in_Length.z * 0.5f);
            maxExtents.set(in_Length.x * 0.5f, in_Length.y * 0.5f, in_Length.z * 0.5f);
        }

        template<class T>
        inline void OBox3<T>::setCenter(const OPoint3 <T> &center) {
            T halflenx = len_x() * 0.5f;
            T halfleny = len_y() * 0.5f;
            T halflenz = len_z() * 0.5f;

            minExtents.set(center.x - halflenx, center.y - halfleny, center.z - halflenz);
            maxExtents.set(center.x + halflenx, center.y + halfleny, center.z + halflenz);
        }

        template<class T>
        inline bool OBox3<T>::isContained(const OPoint3 <T> &in_rContained) const {
            return (in_rContained.x >= minExtents.x && in_rContained.x < maxExtents.x) &&
                   (in_rContained.y >= minExtents.y && in_rContained.y < maxExtents.y) &&
                   (in_rContained.z >= minExtents.z && in_rContained.z < maxExtents.z);
        }

        template<class T>
        inline bool OBox3<T>::isContained(const OPoint2<T> &pt) const {
            return (pt.x >= minExtents.x && pt.x < maxExtents.x) &&
                   (pt.y >= minExtents.y && pt.y < maxExtents.y);
        }

        template<class T>
        inline bool OBox3<T>::isOverlapped(const OBox3 &in_rOverlap) const {
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

        template<class T>
        inline bool OBox3<T>::isContained(const OBox3 &in_rContained) const {
            return (minExtents.x <= in_rContained.minExtents.x) &&
                   (minExtents.y <= in_rContained.minExtents.y) &&
                   (minExtents.z <= in_rContained.minExtents.z) &&
                   (maxExtents.x >= in_rContained.maxExtents.x) &&
                   (maxExtents.y >= in_rContained.maxExtents.y) &&
                   (maxExtents.z >= in_rContained.maxExtents.z);
        }

        template<class T>
        inline void OBox3<T>::intersect(const OBox3 &in_rIntersect) {
            minExtents.setMin(in_rIntersect.minExtents);
            maxExtents.setMax(in_rIntersect.maxExtents);
        }

        template<class T>
        inline void OBox3<T>::intersect(const OPoint3 <T> &in_rIntersect) {
            minExtents.setMin(in_rIntersect);
            maxExtents.setMax(in_rIntersect);
        }

        template<class T>
        inline OBox3<T> OBox3<T>::getOverlap(const OBox3<T> &otherBox) const {
            OBox3 overlap;

            for (unsigned int i = 0; i < 3; ++i) {
                if (minExtents[i] > otherBox.maxExtents[i] || otherBox.minExtents[i] > maxExtents[i]) {
                    overlap.minExtents[i] = 0.f;
                    overlap.maxExtents[i] = 0.f;
                } else {
                    overlap.minExtents[i] = getMax(minExtents[i], otherBox.minExtents[i]);
                    overlap.maxExtents[i] = getMin(maxExtents[i], otherBox.maxExtents[i]);
                }
            }

            return overlap;
        }

        template<class T>
        inline T OBox3<T>::getVolume() const {
            return (maxExtents.x - minExtents.x) * (maxExtents.y - minExtents.y) * (maxExtents.z - minExtents.z);
        }

        template<class T>
        inline void OBox3<T>::getCenter(OPoint3 <T> *center) const {
            center->x = (minExtents.x + maxExtents.x) * 0.5f;
            center->y = (minExtents.y + maxExtents.y) * 0.5f;
            center->z = (minExtents.z + maxExtents.z) * 0.5f;
        }

        template<class T>
        inline OPoint3 <T> OBox3<T>::getCenter() const {
            OPoint3<T> center;
            center.x = (minExtents.x + maxExtents.x) * 0.5f;
            center.y = (minExtents.y + maxExtents.y) * 0.5f;
            center.z = (minExtents.z + maxExtents.z) * 0.5f;
            return center;
        }

        template<class T>
        inline OPoint3 <T> OBox3<T>::getClosestPoint(const OPoint3 <T> &refPt) const {
            OPoint3<T> closest;
            if (refPt.x <= minExtents.x) closest.x = minExtents.x;
            else if (refPt.x > maxExtents.x) closest.x = maxExtents.x;
            else closest.x = refPt.x;

            if (refPt.y <= minExtents.y) closest.y = minExtents.y;
            else if (refPt.y > maxExtents.y) closest.y = maxExtents.y;
            else closest.y = refPt.y;

            if (refPt.z <= minExtents.z) closest.z = minExtents.z;
            else if (refPt.z > maxExtents.z) closest.z = maxExtents.z;
            else closest.z = refPt.z;

            return closest;
        }

        template<class T>
        inline T OBox3<T>::getDistanceToPoint(const OPoint3 <T> &refPt) const {
            return mSqrt(getSqDistanceToPoint(refPt));
        }

        template<class T>
        inline T OBox3<T>::getSqDistanceToPoint(const OPoint3 <T> &refPt) const {
            T sqDist = 0.0f;

            for (unsigned int i = 0; i < 3; i++) {
                const T v = refPt[i];
                if (v < minExtents[i])
                    sqDist += mSquared(minExtents[i] - v);
                else if (v > maxExtents[i])
                    sqDist += mSquared(v - maxExtents[i]);
            }

            return sqDist;
        }

        template<class T>
        inline void OBox3<T>::extend(const OPoint3 <T> &p) {
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

        template<class T>
        inline void OBox3<T>::scale(const OPoint3 <T> &amt) {
            minExtents *= amt;
            maxExtents *= amt;
        }

        template<class T>
        inline void OBox3<T>::scale(T amt) {
            minExtents *= amt;
            maxExtents *= amt;
        }

        template<class T>
        inline bool OBox3<T>::operator==(const OBox3 &b) const {
            return minExtents.equal(b.minExtents) && maxExtents.equal(b.maxExtents);
        }

        template<class T>
        inline bool OBox3<T>::operator!=(const OBox3 &b) const {
            return !minExtents.equal(b.minExtents) || !maxExtents.equal(b.maxExtents);
        }
    };
}