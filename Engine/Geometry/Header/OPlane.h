#pragma once

#include "OPoint3.h"
#include "OMath.h"

namespace omega {
    namespace geometry {

        enum class PlaneSide {
            Front = 1,
            On = 0,
            Back = -1
        };


        template <class T>
        class OPlane : public OPoint3<T> {
        public:
            T x;
            T y;
            T z;
            T d;

            OPlane();

            OPlane(const OPoint3<T> &p, const OPoint3<T> &n);

            OPlane(T _x, T _y, T _z, T _d);

            OPlane(const OPoint3<T> &j, const OPoint3<T> &k, const OPoint3<T> &l);

            void set(const T _x, const T _y, const T _z);

            void set(const OPoint3<T> &p, const OPoint3<T> &n);

            void set(const OPoint3<T> &k, const OPoint3<T> &j, const OPoint3<T> &l);

            void setPoint(const OPoint3<T> &p);

            void setXY(T zz);

            void setYZ(T xx);

            void setXZ(T yy);

            void setXY(const OPoint3<T> &P, T dir);

            void setYZ(const OPoint3<T> &P, T dir);

            void setXZ(const OPoint3<T> &P, T dir);

            void shiftX(T xx);

            void shiftY(T yy);

            void shiftZ(T zz);

            void invert();

            void neg();

            OPoint3<T> project(const OPoint3<T> &pt) const;

            T distToPlane(const OPoint3<T> &cp) const;

            PlaneSide whichSide(const OPoint3<T> &cp) const;

            T intersect(const OPoint3<T> &start, const OPoint3<T> &end) const;

            bool isHorizontal() const;

            bool isVertical() const;

            PlaneSide whichSideBox(const OPoint3<T> &center,
                              const OPoint3<T> &axisx,
                              const OPoint3<T> &axisy,
                              const OPoint3<T> &axisz) const;
        };


        template <class T>
        inline OPlane<T>::OPlane() {
        }

        template <class T>
        inline OPlane<T>::OPlane(T _x, T _y, T _z, T _d) {
            x = _x;
            y = _y;
            z = _z;
            d = _d;
        }

        template <class T>
        inline OPlane<T>::OPlane(const OPoint3<T> &p, const OPoint3<T> &n) {
            set(p, n);
        }

        template <class T>
        inline OPlane<T>::OPlane(const OPoint3<T> &j, const OPoint3<T> &k, const OPoint3<T> &l) {
            set(j, k, l);
        }

        template <class T>
        inline void OPlane<T>::setXY(T zz) {
            x = y = 0;
            z = 1;
            d = -zz;
        }

        template <class T>
        inline void OPlane<T>::setYZ(T xx) {
            x = 1;
            z = y = 0;
            d = -xx;
        }

        template <class T>
        inline void OPlane<T>::setXZ(T yy) {
            x = z = 0;
            y = 1;
            d = -yy;
        }

        template <class T>
        inline void OPlane<T>::setXY(const OPoint3<T> &point, T dir)       // Normal = (0, 0, -1|1)
        {
            x = y = 0;
            d = -((z = dir) * point.z);
        }

        template <class T>
        inline void OPlane<T>::setYZ(const OPoint3<T> &point, T dir)       // Normal = (-1|1, 0, 0)
        {
            z = y = 0;
            d = -((x = dir) * point.x);
        }

        template <class T>
        inline void OPlane<T>::setXZ(const OPoint3<T> &point, T dir)       // Normal = (0, -1|1, 0)
        {
            x = z = 0;
            d = -((y = dir) * point.y);
        }

        template <class T>
        inline void OPlane<T>::shiftX(T xx) {
            d -= xx * x;
        }

        template <class T>
        inline void OPlane<T>::shiftY(T yy) {
            d -= yy * y;
        }

        template <class T>
        inline void OPlane<T>::shiftZ(T zz) {
            d -= zz * z;
        }

        template <class T>
        inline bool OPlane<T>::isHorizontal() const {
            return (x == 0 && y == 0) ? true : false;
        }

        template <class T>
        inline bool OPlane<T>::isVertical() const {
            return ((x != 0 || y != 0) && z == 0) ? true : false;
        }

        template <class T>
        inline OPoint3<T> OPlane<T>::project(const OPoint3<T> &pt) const {
            T dist = distToPlane(pt);
            return OPoint3<T>(pt.x - x * dist, pt.y - y * dist, pt.z - z * dist);
        }

        template <class T>
        inline T OPlane<T>::distToPlane(const OPoint3<T> &cp) const {
            return (x * cp.x + y * cp.y + z * cp.z) + d;
        }

        template <class T>
        inline PlaneSide OPlane<T>::whichSide(const OPoint3<T> &cp) const {
            T dist = distToPlane(cp);
            if (dist >= 0.005f)                 // if (mFabs(dist) < 0.005f)
                return PlaneSide::Front;                    //    return On;
            else if (dist <= -0.005f)           // else if (dist > 0.0f)
                return PlaneSide::Back;                     //    return front;
            else                                // else
                return PlaneSide::On;                       //    return Back;
        }

        template <class T>
        inline void OPlane<T>::set(const T _x, const T _y, const T _z) {
            OPoint3<T>::set(_x, _y, _z);
        }


        template <class T>
        inline void OPlane<T>::setPoint(const OPoint3<T> &p) {
            d = -(p.x * x + p.y * y + p.z * z);
        }

        template <class T>
        inline void OPlane<T>::set(const OPoint3<T> &p, const OPoint3<T> &n) {
            x = n.x;
            y = n.y;
            z = n.z;

            this->normalize();

            d = -(p.x * x + p.y * y + p.z * z);
        }

        template <class T>
        inline void OPlane<T>::set(const OPoint3<T> &k, const OPoint3<T> &j, const OPoint3<T> &l) {
            T ax = k.x - j.x;
            T ay = k.y - j.y;
            T az = k.z - j.z;
            T bx = l.x - j.x;
            T by = l.y - j.y;
            T bz = l.z - j.z;
            x = ay * bz - az * by;
            y = az * bx - ax * bz;
            z = ax * by - ay * bx;
            T squared = x * x + y * y + z * z;

            // In non-debug mode
            if (squared != 0) {
                T invSqrt = 1.0 / (T) mSqrtD(x * x + y * y + z * z);
                x *= invSqrt;
                y *= invSqrt;
                z *= invSqrt;
                d = -(j.x * x + j.y * y + j.z * z);
            } else {
                x = 0;
                y = 0;
                z = 1;
                d = -(j.x * x + j.y * y + j.z * z);
            }
        }

        template <class T>
        inline void OPlane<T>::invert() {
            x = -x;
            y = -y;
            z = -z;
            d = -d;
        }

        template <class T>
        inline void OPlane<T>::neg() {
            invert();
        }

        template <class T>
        inline T OPlane<T>::intersect(const OPoint3<T> &p1, const OPoint3<T> &p2) const {
            T den = mDot(p2 - p1, *this);
            if (den == 0)
                return PARALLEL_PLANE;
            return -distToPlane(p1) / den;
        }

        template <class T>
        inline PlaneSide OPlane<T>::whichSideBox(const OPoint3<T> &center,
                                                 const OPoint3<T> &axisx,
                                                 const OPoint3<T> &axisy,
                                                 const OPoint3<T> &axisz) const {
            T baseDist = distToPlane(center);

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