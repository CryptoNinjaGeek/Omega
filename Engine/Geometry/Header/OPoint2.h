//
// Created by Carsten Tang on 20/03/2022.
//

#pragma once

namespace omega {
    namespace geometry {
        template <class T>
        class OPoint2 {
        public:
            OPoint2();
            OPoint2(const OPoint2&);
            OPoint2(T _x, T _y);

        public:
            void set(T _x, T _y);

            void setMin(const OPoint2&);
            void setMax(const OPoint2&);

            void interpolate(const OPoint2 &a, const OPoint2 &b, const T c);

            void zero();

            operator T*() { return &x; }
            operator const T*() const { return &x; }

        public:
            bool  isZero() const;
            T len()    const;
            T lenSquared() const;

        public:
            void neg();
            void normalize();
            void normalize(T val);
            void convolve(const OPoint2&);
            void convolveInverse(const OPoint2&);

        public:
            // Comparison operators
            bool operator==(const OPoint2&) const;
            bool operator!=(const OPoint2&) const;

            // Arithmetic w/ other points
            OPoint2  operator+(const OPoint2&) const;
            OPoint2  operator-(const OPoint2&) const;
            OPoint2& operator+=(const OPoint2&);
            OPoint2& operator-=(const OPoint2&);

            OPoint2  operator*(T) const;
            OPoint2  operator/(T) const;
            OPoint2& operator*=(T);
            OPoint2& operator/=(T);

            OPoint2 operator-() const;

        public:
            const static OPoint2 One;
            const static OPoint2 Zero;

        public:
            T x;
            T y;
        };

        template <class T>
        OPoint2<T>::OPoint2()
        {
        }


        template <class T>
        OPoint2<T>::OPoint2(const OPoint2<T>& _copy)
                : x(_copy.x), y(_copy.y)
        {
        }


        template <class T>
        OPoint2<T>::OPoint2(T _x, T _y)
                : x(_x), y(_y)
        {
        }


        template <class T>
        void OPoint2<T>::set(T _x, T _y)
        {
            x = _x;
            y = _y;
        }


        template <class T>
        void OPoint2<T>::setMin(const OPoint2<T>& _test)
        {
            x = (_test.x < x) ? _test.x : x;
            y = (_test.y < y) ? _test.y : y;
        }


        template <class T>
        void OPoint2<T>::setMax(const OPoint2<T>& _test)
        {
            x = (_test.x > x) ? _test.x : x;
            y = (_test.y > y) ? _test.y : y;
        }


        template <class T>
        void OPoint2<T>::interpolate(const OPoint2<T>& _rFrom, const OPoint2<T>& _to, const T _factor)
        {
            x = (_rFrom.x * (1.0f - _factor)) + (_to.x * _factor);
            y = (_rFrom.y * (1.0f - _factor)) + (_to.y * _factor);
        }


        template <class T>
        void OPoint2<T>::zero()
        {
            x = y = 0.0;
        }


        template <class T>
        bool OPoint2<T>::isZero() const
        {
            return (x == 0.0f) && (y == 0.0f);
        }


        template <class T>
        T OPoint2<T>::lenSquared() const
        {
            return (x * x) + (y * y);
        }


        template <class T>
        void OPoint2<T>::neg()
        {
            x = -x;
            y = -y;
        }

        template <class T>
        void OPoint2<T>::convolve(const OPoint2<T>& c)
        {
            x *= c.x;
            y *= c.y;
        }

        template <class T>
        void OPoint2<T>::convolveInverse(const OPoint2<T>& c)
        {
            x /= c.x;
            y /= c.y;
        }

        template <class T>
        bool OPoint2<T>::operator==(const OPoint2<T>& _test) const
        {
            return (x == _test.x) && (y == _test.y);
        }


        template <class T>
        bool OPoint2<T>::operator!=(const OPoint2<T>& _test) const
        {
            return operator==(_test) == false;
        }


        template <class T>
        OPoint2<T> OPoint2<T>::operator+(const OPoint2<T>& _add) const
        {
            return OPoint2<T>(x + _add.x, y + _add.y);
        }


        template <class T>
        OPoint2<T> OPoint2<T>::operator-(const OPoint2<T>& _rSub) const
        {
            return OPoint2<T>(x - _rSub.x, y - _rSub.y);
        }


        template <class T>
        OPoint2<T>& OPoint2<T>::operator+=(const OPoint2<T>& _add)
        {
            x += _add.x;
            y += _add.y;

            return *this;
        }


        template <class T>
        OPoint2<T>& OPoint2<T>::operator-=(const OPoint2<T>& _rSub)
        {
            x -= _rSub.x;
            y -= _rSub.y;

            return *this;
        }

        template <class T>
        OPoint2<T> OPoint2<T>::operator*(T _mul) const
        {
            return OPoint2<T>(x * _mul, y * _mul);
        }


        template <class T>
        OPoint2<T> OPoint2<T>::operator/(T _div) const
        {
            T inv = 1.0f / _div;

            return OPoint2<T>(x * inv, y * inv);
        }


        template <class T>
        OPoint2<T>& OPoint2<T>::operator*=(T _mul)
        {
            x *= _mul;
            y *= _mul;

            return *this;
        }


        template <class T>
        OPoint2<T>& OPoint2<T>::operator/=(T _div)
        {
            T inv = 1.0f / _div;

            x *= inv;
            y *= inv;

            return *this;
        }


        template <class T>
        OPoint2<T> OPoint2<T>::operator-() const
        {
            return OPoint2<T>(-x, -y);
        }

        template <class T>
        T OPoint2<T>::len() const
        {
            return mSqrtD(x*x + y*y);
        }

        template <class T>
        void OPoint2<T>::normalize()
        {
            m_OPoint2_normalize(*this);
        }

        template <class T>
        void OPoint2<T>::normalize(T val)
        {
            m_OPoint2_normalize_f(*this, val);
        }


        template <class T>
        OPoint2<T> operator*(T mul, const OPoint2<T>& multiplicand)
        {
            return multiplicand * mul;
        }


        template <class T>
        T mCross(const OPoint2<T> &p0, const OPoint2<T> &p1, const OPoint2<T> &pt2)
        {
            return (p1.x - p0.x) * (pt2.y - p0.y) - (p1.y - p0.y) * (pt2.x - p0.x);
        }
    }
}

