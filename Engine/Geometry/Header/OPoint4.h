#pragma once

#include "OMath.h"
#include "OPoint4.h"

namespace omega {
    namespace geometry {
        template<class T>
        class OPoint4 {
        public:
            T x;   ///< X co-ordinate.
            T y;   ///< Y co-ordinate.
            T z;   ///< Z co-ordinate.
            T w;   ///< W co-ordinate.

        public:
            OPoint4();               ///< Create an uninitialized point.
            OPoint4(const OPoint4 &); ///< Copy constructor.

            /// Create point from coordinates.
            OPoint4(T _x, T _y, T _z, T _w);

            /// Set point's coordinates.
            void set(T _x, T _y, T _z, T _w);

            /// Interpolate from _pt1 to _pt2, based on _factor.
            ///
            /// @param   _pt1    Starting point.
            /// @param   _pt2    Ending point.
            /// @param   _factor Interpolation factor (0.0 .. 1.0).
            void interpolate(const OPoint4 &_pt1, const OPoint4 &_pt2, T _factor);

            void zero();

            operator T *() { return (&x); }

            operator const T *() const { return &x; }

            T len() const;

            OPoint4 operator/(T) const;

            OPoint4 operator*(T) const;

            OPoint4 operator+(const OPoint4 &) const;

            OPoint4 &operator+=(const OPoint4 &);

            OPoint4 operator-(const OPoint4 &) const;

            OPoint4 operator*(const OPoint4 &) const;

            OPoint4 &operator*=(const OPoint4 &);

            OPoint4 &operator=(const OPoint3 <T> &);

            OPoint4 &operator=(const OPoint4 &);

            OPoint3 <T> asPoint3F() const { return OPoint3<T>(x, y, z); }

            void dump(char* title = nullptr);


            //-------------------------------------- Public static constants
        public:
            const static OPoint4 One;
            const static OPoint4 Zero;
        };

        typedef OPoint4<float> Vector4F;   ///< Points can be vectors!

        template<class T>
        inline OPoint4<T>::OPoint4() {
        }

        template<class T>
        inline OPoint4<T>::OPoint4(const OPoint4 &_copy)
                : x(_copy.x), y(_copy.y), z(_copy.z), w(_copy.w) {
        }

        template<class T>
        inline OPoint4<T>::OPoint4(T _x, T _y, T _z, T _w)
                : x(_x), y(_y), z(_z), w(_w) {
        }

        template<class T>
        inline void OPoint4<T>::set(T _x, T _y, T _z, T _w) {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
        }

        template<class T>
        inline T OPoint4<T>::len() const {
            return mSqrt(x * x + y * y + z * z + w * w);
        }

        template<class T>
        inline void OPoint4<T>::interpolate(const OPoint4 &_from, const OPoint4 &_to, T _factor) {
            x = (_from.x * (1.0f - _factor)) + (_to.x * _factor);
            y = (_from.y * (1.0f - _factor)) + (_to.y * _factor);
            z = (_from.z * (1.0f - _factor)) + (_to.z * _factor);
            w = (_from.w * (1.0f - _factor)) + (_to.w * _factor);
        }

        template<class T>
        inline void OPoint4<T>::zero() {
            x = y = z = w = 0.0f;
        }

        template<class T>
        inline OPoint4<T> &OPoint4<T>::operator=(const OPoint3 <T> &_vec) {
            x = _vec.x;
            y = _vec.y;
            z = _vec.z;
            w = 1.0f;
            return *this;
        }

        template<class T>
        inline OPoint4<T> &OPoint4<T>::operator=(const OPoint4<T> &_vec) {
            x = _vec.x;
            y = _vec.y;
            z = _vec.z;
            w = _vec.w;

            return *this;
        }

        template<class T>
        inline OPoint4<T> OPoint4<T>::operator+(const OPoint4<T> &_add) const {
            return OPoint4(x + _add.x, y + _add.y, z + _add.z, w + _add.w);
        }

        template <class T>
        inline void OPoint4<T>::dump(char* title)
        {
            if (title)
                std::cout << std::fixed << std::setw(11) << std::setfill(' ') << title << " => ";

            std::cout << x << " , " << y << " , " << z  << " , " << w << std::endl;
        }


        template<class T>
        inline OPoint4<T> &OPoint4<T>::operator+=(const OPoint4<T> &_add) {
            x += _add.x;
            y += _add.y;
            z += _add.z;
            w += _add.w;

            return *this;
        }

        template<class T>
        inline OPoint4<T> OPoint4<T>::operator-(const OPoint4<T> &_rSub) const {
            return OPoint4(x - _rSub.x, y - _rSub.y, z - _rSub.z, w - _rSub.w);
        }

        template<class T>
        inline OPoint4<T> OPoint4<T>::operator*(const OPoint4<T> &_vec) const {
            return OPoint4(x * _vec.x, y * _vec.y, z * _vec.z, w * _vec.w);
        }

        template<class T>
        inline OPoint4<T> OPoint4<T>::operator*(T _mul) const {
            return OPoint4(x * _mul, y * _mul, z * _mul, w * _mul);
        }

        template<class T>
        inline OPoint4<T> OPoint4<T>::operator/(T t) const {
            T f = 1.0f / t;
            return OPoint4(x * f, y * f, z * f, w * f);
        }

        template<class T>
        inline OPoint4<T> operator*(T mul, const OPoint4<T> &multiplicand) {
            return multiplicand * mul;
        }

        template<class T>
        inline bool mIsNaN(const OPoint4<T> &p) {
            return mIsNaN_F(p.x) || mIsNaN_F(p.y) || mIsNaN_F(p.z) || mIsNaN_F(p.w);
        }

        template <class T>
        OPoint4<T> mRotate(const OPoint4<T> &u, float angle, const OPoint3<T> &v)
        {
            return *(OPoint4<T>*)&(mRotate(angle, v) * u);
        }

    }
}
