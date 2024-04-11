#pragma once

#include <geometry/Math.h>
#include <geometry/Point4.h>

namespace omega {
namespace geometry {
template<class T>
class Point4 {
public:
  T x;   ///< X co-ordinate.
  T y;   ///< Y co-ordinate.
  T z;   ///< Z co-ordinate.
  T w;   ///< W co-ordinate.

public:
  Point4();               ///< Create an uninitialized point.
  Point4(const Point4 &); ///< Copy constructor.

  /// Create point from coordinates.
  Point4(T _x, T _y, T _z, T _w);

  /// Set point's coordinates.
  void set(T _x, T _y, T _z, T _w);

  /// Interpolate from _pt1 to _pt2, based on _factor.
  ///
  /// @param   _pt1    Starting point.
  /// @param   _pt2    Ending point.
  /// @param   _factor Interpolation factor (0.0 .. 1.0).
  void interpolate(const Point4 &_pt1, const Point4 &_pt2, T _factor);

  void zero();

  operator T *() { return (&x); }

  operator const T *() const { return &x; }

  T len() const;

  Point4 operator/(T) const;

  Point4 operator*(T) const;

  Point4 operator+(const Point4 &) const;

  Point4 &operator+=(const Point4 &);

  Point4 operator-(const Point4 &) const;

  Point4 operator*(const Point4 &) const;

  Point4 &operator*=(const Point4 &);

  Point4 &operator=(const Point3<T> &);

  Point4 &operator=(const Point4 &);

  Point3<T> asPoint3F() const { return Point3<T>(x, y, z); }

  void dump(char *title = nullptr);


  //-------------------------------------- Public static constants
public:
  const static Point4 One;
  const static Point4 Zero;
};

typedef Point4<float> Vector4F;   ///< Points can be vectors!

template<class T>
inline Point4<T>::Point4() {
}

template<class T>
inline Point4<T>::Point4(const Point4 &_copy)
	: x(_copy.x), y(_copy.y), z(_copy.z), w(_copy.w) {
}

template<class T>
inline Point4<T>::Point4(T _x, T _y, T _z, T _w)
	: x(_x), y(_y), z(_z), w(_w) {
}

template<class T>
inline void Point4<T>::set(T _x, T _y, T _z, T _w) {
  x = _x;
  y = _y;
  z = _z;
  w = _w;
}

template<class T>
inline T Point4<T>::len() const {
  return mSqrt(x*x + y*y + z*z + w*w);
}

template<class T>
inline void Point4<T>::interpolate(const Point4 &_from, const Point4 &_to, T _factor) {
  x = (_from.x*(1.0f - _factor)) + (_to.x*_factor);
  y = (_from.y*(1.0f - _factor)) + (_to.y*_factor);
  z = (_from.z*(1.0f - _factor)) + (_to.z*_factor);
  w = (_from.w*(1.0f - _factor)) + (_to.w*_factor);
}

template<class T>
inline void Point4<T>::zero() {
  x = y = z = w = 0.0f;
}

template<class T>
inline Point4<T> &Point4<T>::operator=(const Point3<T> &_vec) {
  x = _vec.x;
  y = _vec.y;
  z = _vec.z;
  w = 1.0f;
  return *this;
}

template<class T>
inline Point4<T> &Point4<T>::operator=(const Point4<T> &_vec) {
  x = _vec.x;
  y = _vec.y;
  z = _vec.z;
  w = _vec.w;

  return *this;
}

template<class T>
inline Point4<T> Point4<T>::operator+(const Point4<T> &_add) const {
  return Point4(x + _add.x, y + _add.y, z + _add.z, w + _add.w);
}

template<class T>
inline void Point4<T>::dump(char *title) {
  if (title)
	std::cout << std::fixed << std::setw(11) << std::setfill(' ') << title << " => ";

  std::cout << x << " , " << y << " , " << z << " , " << w << std::endl;
}

template<class T>
inline Point4<T> &Point4<T>::operator+=(const Point4<T> &_add) {
  x += _add.x;
  y += _add.y;
  z += _add.z;
  w += _add.w;

  return *this;
}

template<class T>
inline Point4<T> Point4<T>::operator-(const Point4<T> &_rSub) const {
  return Point4(x - _rSub.x, y - _rSub.y, z - _rSub.z, w - _rSub.w);
}

template<class T>
inline Point4<T> Point4<T>::operator*(const Point4<T> &_vec) const {
  return Point4(x*_vec.x, y*_vec.y, z*_vec.z, w*_vec.w);
}

template<class T>
inline Point4<T> Point4<T>::operator*(T _mul) const {
  return Point4(x*_mul, y*_mul, z*_mul, w*_mul);
}

template<class T>
inline Point4<T> Point4<T>::operator/(T t) const {
  T f = 1.0f/t;
  return Point4(x*f, y*f, z*f, w*f);
}

template<class T>
inline Point4<T> operator*(T mul, const Point4<T> &multiplicand) {
  return multiplicand*mul;
}

template<class T>
inline bool mIsNaN(const Point4<T> &p) {
  return mIsNaN_F(p.x) || mIsNaN_F(p.y) || mIsNaN_F(p.z) || mIsNaN_F(p.w);
}

template<class T>
Point4<T> mRotate(const Point4<T> &u, float angle, const Point3<T> &v) {
  return *(Point4<T> *)&(mRotate(angle, v)*u);
}

}
}
