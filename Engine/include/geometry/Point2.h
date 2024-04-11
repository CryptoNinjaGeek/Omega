//
// Created by Carsten Tang on 20/03/2022.
//

#pragma once

namespace omega {
namespace geometry {
template<class T>
class Point2 {
public:
  Point2();
  Point2(const Point2 &);
  Point2(T _x, T _y);

public:
  void set(T _x, T _y);

  void setMin(const Point2 &);
  void setMax(const Point2 &);

  void interpolate(const Point2 &a, const Point2 &b, const T c);

  void zero();

  operator T *() { return &x; }
  operator const T *() const { return &x; }

public:
  bool isZero() const;
  T len() const;
  T lenSquared() const;

public:
  void neg();
  void normalize();
  void normalize(T val);
  void convolve(const Point2 &);
  void convolveInverse(const Point2 &);

public:
  // Comparison operators
  bool operator==(const Point2 &) const;
  bool operator!=(const Point2 &) const;

  // Arithmetic w/ other points
  Point2 operator+(const Point2 &) const;
  Point2 operator-(const Point2 &) const;
  Point2 &operator+=(const Point2 &);
  Point2 &operator-=(const Point2 &);

  Point2 operator*(T) const;
  Point2 operator/(T) const;
  Point2 &operator*=(T);
  Point2 &operator/=(T);

  Point2 operator-() const;

public:
  const static Point2 One;
  const static Point2 Zero;

public:
  T x;
  T y;
};

template<class T>
Point2<T>::Point2() {
}

template<class T>
Point2<T>::Point2(const Point2<T> &_copy)
	: x(_copy.x), y(_copy.y) {
}

template<class T>
Point2<T>::Point2(T _x, T _y)
	: x(_x), y(_y) {
}

template<class T>
void Point2<T>::set(T _x, T _y) {
  x = _x;
  y = _y;
}

template<class T>
void Point2<T>::setMin(const Point2<T> &_test) {
  x = (_test.x < x) ? _test.x : x;
  y = (_test.y < y) ? _test.y : y;
}

template<class T>
void Point2<T>::setMax(const Point2<T> &_test) {
  x = (_test.x > x) ? _test.x : x;
  y = (_test.y > y) ? _test.y : y;
}

template<class T>
void Point2<T>::interpolate(const Point2<T> &_rFrom, const Point2<T> &_to, const T _factor) {
  x = (_rFrom.x*(1.0f - _factor)) + (_to.x*_factor);
  y = (_rFrom.y*(1.0f - _factor)) + (_to.y*_factor);
}

template<class T>
void Point2<T>::zero() {
  x = y = 0.0;
}

template<class T>
bool Point2<T>::isZero() const {
  return (x==0.0f) && (y==0.0f);
}

template<class T>
T Point2<T>::lenSquared() const {
  return (x*x) + (y*y);
}

template<class T>
void Point2<T>::neg() {
  x = -x;
  y = -y;
}

template<class T>
void Point2<T>::convolve(const Point2<T> &c) {
  x *= c.x;
  y *= c.y;
}

template<class T>
void Point2<T>::convolveInverse(const Point2<T> &c) {
  x /= c.x;
  y /= c.y;
}

template<class T>
bool Point2<T>::operator==(const Point2<T> &_test) const {
  return (x==_test.x) && (y==_test.y);
}

template<class T>
bool Point2<T>::operator!=(const Point2<T> &_test) const {
  return operator==(_test)==false;
}

template<class T>
Point2<T> Point2<T>::operator+(const Point2<T> &_add) const {
  return Point2<T>(x + _add.x, y + _add.y);
}

template<class T>
Point2<T> Point2<T>::operator-(const Point2<T> &_rSub) const {
  return Point2<T>(x - _rSub.x, y - _rSub.y);
}

template<class T>
Point2<T> &Point2<T>::operator+=(const Point2<T> &_add) {
  x += _add.x;
  y += _add.y;

  return *this;
}

template<class T>
Point2<T> &Point2<T>::operator-=(const Point2<T> &_rSub) {
  x -= _rSub.x;
  y -= _rSub.y;

  return *this;
}

template<class T>
Point2<T> Point2<T>::operator*(T _mul) const {
  return Point2<T>(x*_mul, y*_mul);
}

template<class T>
Point2<T> Point2<T>::operator/(T _div) const {
  T inv = 1.0f/_div;

  return Point2<T>(x*inv, y*inv);
}

template<class T>
Point2<T> &Point2<T>::operator*=(T _mul) {
  x *= _mul;
  y *= _mul;

  return *this;
}

template<class T>
Point2<T> &Point2<T>::operator/=(T _div) {
  T inv = 1.0f/_div;

  x *= inv;
  y *= inv;

  return *this;
}

template<class T>
Point2<T> Point2<T>::operator-() const {
  return Point2<T>(-x, -y);
}

template<class T>
T Point2<T>::len() const {
  return mSqrtD(x*x + y*y);
}

template<class T>
void Point2<T>::normalize() {
  m_Point2_normalize(*this);
}

template<class T>
void Point2<T>::normalize(T val) {
  m_Point2_normalize_f(*this, val);
}

template<class T>
Point2<T> operator*(T mul, const Point2<T> &multiplicand) {
  return multiplicand*mul;
}

template<class T>
T mCross(const Point2<T> &p0, const Point2<T> &p1, const Point2<T> &pt2) {
  return (p1.x - p0.x)*(pt2.y - p0.y) - (p1.y - p0.y)*(pt2.x - p0.x);
}
}
}

