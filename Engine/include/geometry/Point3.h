//
// Created by Carsten Tang on 20/03/2022.
//

#pragma once

#include <geometry/Math.h>
#include <iostream>
#include <iomanip>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include <geometry/Quaternion.h>

namespace omega {
    namespace geometry {
        template <class T>
        class OPoint3 {
        public:
            T x = {};
            T y = {};
            T z = {};

        public:
            OPoint3();
            OPoint3(const OPoint3&);
            explicit OPoint3(T xyz);
            OPoint3(T _x, T _y, T _z);
            OPoint3(T v[3]);

        public:
            void set(T xyz);
            void set(T _x, T _y, T _z);

            void setMin(const OPoint3&);
            void setMax(const OPoint3&);

            void interpolate(const OPoint3&, const OPoint3&, T);
            void zero();

            OPoint3 cross(OPoint3);
            void rotate(float, OPoint3);

            operator T*() { return (&x); }
            operator const T*() const { return &x; }
            glm::vec3 toVec3();

            void dump(char *title = nullptr);

        public:
            bool  isZero() const;
            T len()    const;
            T lenSquared() const;
            T magnitudeSafe() const;

        public:
            void neg();
            void normalize();
            void normalizeSafe();
            void normalize(T val);
            void convolve(const OPoint3&);
            void convolveInverse(const OPoint3&);

        public:
            bool operator==(const OPoint3&) const;
            bool operator!=(const OPoint3&) const;

            OPoint3  operator+(const glm::vec3&) const;

            OPoint3  operator+(const OPoint3&) const;
            OPoint3  operator-(const OPoint3&) const;
            OPoint3& operator+=(const OPoint3&);
            OPoint3& operator-=(const OPoint3&);

            OPoint3  operator*(T) const;
            OPoint3  operator/(T) const;
            OPoint3& operator*=(T);
            OPoint3& operator/=(T);

            OPoint3 operator-() const;
            void operator=(glm::vec3);

        public:
            const static OPoint3 One;
            const static OPoint3 Zero;
        };


        template <class T>
        inline OPoint3<T>::OPoint3()
        {
        }

        template <class T>
        inline OPoint3<T>::OPoint3(T v[3])
            : x(v[0]), y(v[1]), z(v[2])
        {
        }

        template <class T>
        inline OPoint3<T>::OPoint3(const OPoint3<T>& _copy)
                : x(_copy.x), y(_copy.y), z(_copy.z)
        {
        }


        template <class T>
        inline OPoint3<T>::OPoint3(T xyz)
                : x(xyz), y(xyz), z(xyz)
        {
        }

        template <class T>
        inline OPoint3<T>::OPoint3(T _x, T _y, T _z)
                : x(_x), y(_y), z(_z)
        {
            //
        }

        template <class T>
        inline void OPoint3<T>::set( T xyz )
        {
            x = y = z = xyz;
        }

        template <class T>
        inline void OPoint3<T>::set(T _x, T _y, T _z)
        {
            x = _x;
            y = _y;
            z = _z;
        }

        template <class T>
        inline void OPoint3<T>::setMin(const OPoint3<T>& _test)
        {
            x = (_test.x < x) ? _test.x : x;
            y = (_test.y < y) ? _test.y : y;
            z = (_test.z < z) ? _test.z : z;
        }

        template <class T>
        inline void OPoint3<T>::setMax(const OPoint3<T>& _test)
        {
            x = (_test.x > x) ? _test.x : x;
            y = (_test.y > y) ? _test.y : y;
            z = (_test.z > z) ? _test.z : z;
        }

        template <class T>
        inline glm::vec3 OPoint3<T>::toVec3()
        {
            return glm::vec3(x,y,z);
        }

        template <class T>
        inline void OPoint3<T>::interpolate(const OPoint3<T>& _from, const OPoint3<T>& _to, T _factor)
        {
            m_point3D_interpolate( _from, _to, _factor, *this);
        }

        template <class T>
        inline void OPoint3<T>::zero()
        {
            x = y = z = 0.0;
        }

        template <class T>
        inline bool OPoint3<T>::isZero() const
        {
            return (x == 0.0f) && (y == 0.0f) && (z == 0.0f);
        }

        template <class T>
        inline void OPoint3<T>::neg()
        {
            x = -x;
            y = -y;
            z = -z;
        }

        template <class T>
        inline void OPoint3<T>::convolve(const OPoint3& c)
        {
            x *= c.x;
            y *= c.y;
            z *= c.z;
        }

        template <class T>
        inline void OPoint3<T>::convolveInverse(const OPoint3& c)
        {
            x /= c.x;
            y /= c.y;
            z /= c.z;
        }

        template <class T>
        inline T OPoint3<T>::lenSquared() const
        {
            return (x * x) + (y * y) + (z * z);
        }

        template <class T>
        inline T OPoint3<T>::len() const
        {
            T temp = x*x + y*y + z*z;
            return (temp > 0.0) ? mSqrt(temp) : 0.0;
        }

        template <class T>
        inline void OPoint3<T>::normalize()
        {
            T factor = 1.0f / mSqrt<T>(x*x + y*y + z*z );
            x *= factor;
            y *= factor;
            z *= factor;
        }

        template <class T>
        inline T OPoint3<T>::magnitudeSafe() const
        {
            if( isZero() )
            {
                return 0.0;
            }
            else
            {
                return len();
            }
        }

        template <class T>
        inline void OPoint3<T>::normalizeSafe()
        {
            T vmag = magnitudeSafe();

            if( vmag > POINT_EPSILON )
            {
                *this *= T(1.0 / vmag);
            }
        }

        template <class T>
        inline void OPoint3<T>::normalize(T val)
        {
            m_point3D_normalize_f(*this, val);
        }

        template <class T>
        inline void OPoint3<T>::operator=(glm::vec3 v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
        }

        template <class T>
        inline bool OPoint3<T>::operator==(const OPoint3<T>& _test) const
        {
            return (x == _test.x) && (y == _test.y) && (z == _test.z);
        }

        template <class T>
        inline bool OPoint3<T>::operator!=(const OPoint3<T>& _test) const
        {
            return operator==(_test) == false;
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator+(const glm::vec3& _add) const
        {
            return OPoint3(x + _add.x, y + _add.y, z + _add.z);
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator+(const OPoint3<T>& _add) const
        {
            return OPoint3(x + _add.x, y + _add.y,  z + _add.z);
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator-(const OPoint3<T>& _rSub) const
        {
            return OPoint3(x - _rSub.x, y - _rSub.y, z - _rSub.z);
        }

        template <class T>
        inline OPoint3<T>& OPoint3<T>::operator+=(const OPoint3<T>& _add)
        {
            x += _add.x;
            y += _add.y;
            z += _add.z;

            return *this;
        }

        template <class T>
        inline OPoint3<T>& OPoint3<T>::operator-=(const OPoint3<T>& _rSub)
        {
            x -= _rSub.x;
            y -= _rSub.y;
            z -= _rSub.z;

            return *this;
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator*(T _mul) const
        {
            return OPoint3(x * _mul, y * _mul, z * _mul);
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator/(T _div) const
        {
            T inv = 1.0f / _div;

            return OPoint3<T>(x * inv, y * inv, z * inv);
        }

        template <class T>
        inline OPoint3<T>& OPoint3<T>::operator*=(T _mul)
        {
            x *= _mul;
            y *= _mul;
            z *= _mul;

            return *this;
        }

        template <class T>
        inline OPoint3<T>& OPoint3<T>::operator/=(T _div)
        {
            T inv = 1.0f / _div;
            x *= inv;
            y *= inv;
            z *= inv;

            return *this;
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::operator-() const
        {
            return OPoint3<T>(-x, -y, -z);
        }

        template <class T>
        inline void OPoint3<T>::dump(char *title)
        {
            if(title)
                std::cout << std::fixed << std::setw(11) << std::setfill(' ') << title << " => ";

            std::cout << x << " , " << y << " , " << z << std::endl;
        }


        template <class T>
        inline OPoint3<T> operator*(T mul, const OPoint3<T>& multiplicand)
        {
            return multiplicand * mul;
        }

        template <class T>
        inline T mDot(const OPoint3<T> &p1, const OPoint3<T> &p2)
        {
            return (p1.x*p2.x + p1.y*p2.y + p1.z*p2.z);
        }

        template <class T>
        inline void mCross(const OPoint3<T> &a, const OPoint3<T> &b, OPoint3<T> *res)
        {
            res->x = (a.y * b.z) - (a.z * b.y);
            res->y = (a.z * b.x) - (a.x * b.z);
            res->z = (a.x * b.y) - (a.y * b.x);
        }

        template <class T>
        inline OPoint3<T> mCross(const OPoint3<T> &a, const OPoint3<T> &b)
        {
            OPoint3<T> r;
            mCross( a, b, &r );
            return r;
        }

        template <class T>
        inline OPoint3<T> OPoint3<T>::cross(OPoint3<T> p)
        {
            OPoint3<T> r;
            mCross(*this, p, &r);
            return r;
        }


        template <class T>
        inline void OPoint3<T>::rotate(float angle, OPoint3<T> v)
        {
            OQuaternion RotationQ(angle, v);

            OQuaternion ConjugateQ = RotationQ.conjugate();

            OQuaternion W = RotationQ * (*this) * ConjugateQ;

            x = W.x;
            y = W.y;
            z = W.z;
        }


        template <class T>
        inline  OPoint3<T> mNormalize(const OPoint3<T> &p)
        {
            T factor = 1.0f / mSqrt<T>(p.x*p.x + p.y*p.y + p.z*p.z );
            return OPoint3<T>( p.x * factor , p.y * factor , p.z * factor );
        }

        template <class T>
        void GetXY(const OPoint3<T> &Z, OPoint3<T> &X, OPoint3<T> &Y)
        {
            if(Z.y == -1.0f)
            {
                X = OPoint3<T>(1.0f, 0.0f, 0.0f);
                Y = OPoint3<T>(0.0f, 0.0f, 1.0f);
            }
            else if(Z.y > -1.0f && Z.y < 0.0f)
            {
                X = mCross(OPoint3<T>(0.0f, 1.0f, 0.0f), Z);
                Y = mCross(Z, X);
            }
            else if(Z.y == 0.0f)
            {
                Y = OPoint3<T>(0.0f, 1.0f, 0.0f);
                X = mCross(Y, Z);
            }
            else if(Z.y > 0.0f && Z.y < 1.0f)
            {
                X = mCross(OPoint3<T>(0.0f, 1.0f, 0.0f), Z);
                Y = mCross(Z, X);
            }
            else if(Z.y == 1.0f)
            {
                X = OPoint3<T>(1.0f, 0.0f, 0.0f);
                Y = OPoint3<T>(0.0f, 0.0f, -1.0f);
            }
        }

        template <class T>
        OPoint3<T> mRotate(const OPoint3<T> &u, float angle, const OPoint3<T> &v)
        {
            return (mRotate(angle, v) * u);
        }

        typedef OPoint3<float> OVector;
        typedef OPoint3<float> EulerF;
    }
}
