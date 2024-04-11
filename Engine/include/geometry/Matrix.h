#pragma once

#include <geometry/Plane.h>
#include <geometry/Point3.h>
#include <geometry/Point4.h>
#include <geometry/Box.h>

#include <iostream>
#include <iomanip>
#include <cmath>

#include "glm/mat4x4.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace omega {
namespace geometry {
template<class T>
class Matrix {
public:
  T m[16];

public:
  explicit Matrix(bool identity = false);
  explicit Matrix(const EulerF &e);
  Matrix(const EulerF &e, const Point3<T> &p);
  Matrix(T m1, T m2, T m3, T m4, T m5, T m6, T m7, T m8, T m9, T m10, T m11,
		 T m12, T m13, T m14, T m15, T m16);

  static int idx(T i, T j) { return (i + j*4); }

  Matrix &set(const EulerF &e);
  Matrix &set(const EulerF &e, const Point3<T> &p);
  Matrix &setCrossProduct(const Point3<T> &p);
  Matrix &setTensorProduct(const Point3<T> &p, const Point3<T> &q);

  operator T *() { return (m); }                  ///< Allow people to get at m.
  operator const T *() const { return (T *)(m); }  ///< Allow people to get at m.

  bool isAffine() const;    ///< Check to see if this is an affine matrix.
  bool isIdentity() const;  ///< Checks for identity matrix.

  /// Make this an identity matrix.
  Matrix &identity();

  /// Invert m.
  Matrix &inverse();

  /// Copy the inversion of this into out matrix.
  void invertTo(Matrix *out);

  /// Take inverse of matrix assuming it is affine (rotation,
  /// scale, sheer, translation only).
  Matrix &affineInverse();

  /// Swap rows and columns.
  Matrix &transpose();

  /// M * Matrix(p) -> M
  Matrix &scale(const Point3<T> &s);

  Matrix &scale(T s) { return scale(Point3<T>(s, s, s)); }

  /// Return scale assuming scale was applied via mat.scale(s).
  Point3<T> getScale() const;

  EulerF toEuler() const;

  /// Compute the inverse of the matrix.
  ///
  /// Computes inverse of full 4x4 matrix. Returns false and performs no inverse
  /// if the determinant is 0.
  ///
  /// Note: In most cases you want to use the normal inverse function.  This
  /// method should
  ///       be used if the matrix has something other than (0,0,0,1) in the
  ///       bottom row.
  bool fullInverse();

  /// Swaps rows and columns into matrix.
  void transposeTo(T *matrix) const;

  /// Normalize the matrix.
  void normalize();

  void getColumn(unsigned int col, Point4<T> *cptr) const;

  Point4<T> getColumn4(unsigned int col) const {
	Point4<T> ret;
	getColumn(col, &ret);
	return ret;
  }

  /// Copy the requested column into a Point3<T>.
  ///
  /// This drops the bottom-most row.
  void getColumn(unsigned int col, Point3<T> *cptr) const;

  Point3<T> getColumn3(unsigned int col) const {
	Point3<T> ret;
	getColumn(col, &ret);
	return ret;
  }

  /// Set the specified column from a Point4<T>.
  void setColumn(unsigned int col, const Point4<T> &cptr);

  /// Set the specified column from a Point3<T>.
  ///
  /// The bottom-most row is not set.
  void setColumn(unsigned int col, const Point3<T> &cptr);

  /// Copy the specified row into a Point4<T>.
  void getRow(unsigned int row, Point4<T> *cptr) const;

  Point4<T> getRow4F(T row) const {
	Point4<T> ret;
	getRow(row, &ret);
	return ret;
  }

  /// Copy the specified row into a Point3<T>.
  ///
  /// right-most item is dropped.
  void getRow(unsigned int row, Point3<T> *cptr) const;

  Point3<T> getRow3F(unsigned int row) const {
	Point3<T> ret;
	getRow(row, &ret);
	return ret;
  }

  /// Set the specified row from a Point4<T>.
  void setRow(unsigned int row, const Point4<T> &cptr);

  /// Set the specified row from a Point3<T>.
  ///
  /// The right-most item is not set.
  void setRow(unsigned int row, const Point3<T> &cptr);

  /// Get the position of the matrix.
  ///
  /// This is the 4th column of the matrix.
  Point3<T> getPosition() const;

  /// Set the position of the matrix.
  ///
  /// This is the 4th column of the matrix.
  void setPosition(const Point3<T> &pos) { setColumn(3, pos); }

  /// Add the passed delta to the matrix position.
  void displace(const Point3<T> &delta);

  /// Get the x axis of the matrix.
  ///
  /// This is the 1st column of the matrix and is
  /// normally considered the right vector.
  Vector getRightVector() const;

  /// Get the y axis of the matrix.
  ///
  /// This is the 2nd column of the matrix and is
  /// normally considered the forward vector.
  Vector getForwardVector() const;

  /// Get the z axis of the matrix.
  ///
  /// This is the 3rd column of the matrix and is
  /// normally considered the up vector.
  Vector getUpVector() const;

  Matrix &mul(const Matrix &a);                    ///< M * a -> M
  Matrix &mulL(const Matrix &a);                   ///< a * M -> M
  Matrix &mul(const Matrix &a, const Matrix &b);  ///< a * b -> M

  // Scalar multiplies
  Matrix &mul(const T a);                    ///< M * a -> M
  Matrix &mul(const Matrix &a, const T b);  ///< a * b -> M

  void mul(Point4<T> &p) const;   ///< M * p -> p (full [4x4] * [1x4])
  void mulP(Point3<T> &p) const;  ///< M * p -> p (assume w = 1.0f)
  void mulP(const Point3<T> &p,
			Point3<T> *d) const;  ///< M * p -> d (assume w = 1.0f)
  void mulV(Vector &p) const;     ///< M * v -> v (assume w = 0.0f)
  void mulV(const Vector &p,
			Point3<T> *d) const;  ///< M * v -> d (assume w = 0.0f)

  void mul(Box3<T> &b) const;  ///< Axial box -> Axial Box

  Matrix &add(const Matrix &m);

  /// Convenience function to allow people to treat this like an array.
  T &operator()(T row, T col) { return m[idx(col, row)]; }

  T operator()(T row, T col) const { return m[idx(col, row)]; }

  void dumpMatrix(const char *caption = nullptr) const;

  // Math operator overloads
  //------------------------------------
  Matrix operator*(const Matrix &m);
  Matrix &operator*=(const Matrix &m);

  void operator=(const glm::mat4x4 m);

  Point3<T> operator*(const Point3<T> pt);
  Point4<T> operator*(const Point4<T> pt);

  // Static identity matrix
  const static Matrix Identity;
};

//--------------------------------------
// Inline Functions

template<class T>
inline Matrix<T>::Matrix(T m1, T m2, T m3, T m4, T m5, T m6, T m7, T m8, T m9,
						 T m10, T m11, T m12, T m13, T m14, T m15, T m16) {
  setColumn(0, Point4<float>(m1, m2, m3, m4));
  setColumn(1, Point4<float>(m5, m6, m7, m8));
  setColumn(2, Point4<float>(m9, m10, m11, m12));
  setColumn(3, Point4<float>(m13, m14, m15, m16));
}

template<class T>
inline Matrix<T>::Matrix(bool _identity) {
  if (_identity)
	identity();
  else
	memset(m, 0, 16*sizeof(T));
}

template<class T>
inline Matrix<T>::Matrix(const EulerF &e) {
  set(e);
}

template<class T>
inline Matrix<T>::Matrix(const EulerF &e, const Point3<T> &p) {
  set(e, p);
}

template<class T>
inline Matrix<T> &Matrix<T>::set(const EulerF &e) {
  m_matF_set_euler(e, *this);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::set(const EulerF &e, const Point3<T> &p) {
  m_matF_set_euler_point(e, p, *this);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::setCrossProduct(const Point3<T> &p) {
  m[1] = -(m[4] = p.z);
  m[8] = -(m[2] = p.y);
  m[6] = -(m[9] = p.x);
  m[0] = m[3] = m[5] = m[7] = m[10] = m[11] = m[12] = m[13] = m[14] = 0.0f;
  m[15] = 1;
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::setTensorProduct(const Point3<T> &p,
											  const Point3<T> &q) {
  m[0] = p.x*q.x;
  m[1] = p.x*q.y;
  m[2] = p.x*q.z;
  m[4] = p.y*q.x;
  m[5] = p.y*q.y;
  m[6] = p.y*q.z;
  m[8] = p.z*q.x;
  m[9] = p.z*q.y;
  m[10] = p.z*q.z;
  m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0f;
  m[15] = 1.0f;
  return (*this);
}

template<class T>
inline bool Matrix<T>::isIdentity() const {
  return m[0]==1.0f && m[1]==0.0f && m[2]==0.0f && m[3]==0.0f &&
	  m[4]==0.0f && m[5]==1.0f && m[6]==0.0f && m[7]==0.0f &&
	  m[8]==0.0f && m[9]==0.0f && m[10]==1.0f && m[11]==0.0f &&
	  m[12]==0.0f && m[13]==0.0f && m[14]==0.0f && m[15]==1.0f;
}

template<class T>
inline Matrix<T> &Matrix<T>::identity() {
  m[0] = 1.0f;
  m[1] = 0.0f;
  m[2] = 0.0f;
  m[3] = 0.0f;
  m[4] = 0.0f;
  m[5] = 1.0f;
  m[6] = 0.0f;
  m[7] = 0.0f;
  m[8] = 0.0f;
  m[9] = 0.0f;
  m[10] = 1.0f;
  m[11] = 0.0f;
  m[12] = 0.0f;
  m[13] = 0.0f;
  m[14] = 0.0f;
  m[15] = 1.0f;
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::inverse() {
  m_mat_inverse<T>(m);
  return (*this);
}

template<class T>
inline void Matrix<T>::invertTo(Matrix<T> *out) {
  m_matF_invert_to(m, *out);
}

template<class T>
inline Matrix<T> &Matrix<T>::affineInverse() {
  m_matF_affineInverse(m);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::transpose() {
  m_matF_transpose(m);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::scale(const Point3<T> &p) {
  m_matF_scale(m, p);
  return *this;
}

template<class T>
inline Point3<T> Matrix<T>::getScale() const {
  Point3<T> scale;
  scale.x = mSqrt(m[0]*m[0] + m[4]*m[4] + m[8]*m[8]);
  scale.y = mSqrt(m[1]*m[1] + m[5]*m[5] + m[9]*m[9]);
  scale.z = mSqrt(m[2]*m[2] + m[6]*m[6] + m[10]*m[10]);
  return scale;
}

template<class T>
inline void Matrix<T>::normalize() {
  m_matF_normalize(m);
}

template<class T>
inline Matrix<T> &Matrix<T>::mul(const Matrix<T> &a) {  // M * a -> M
  AssertFatal(&a!=this, "Matrix<T>::mul - a.mul(a) is invalid!");

  Matrix tempThis(*this);
  m_matF_x_matF(tempThis, a, *this);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::mulL(const Matrix<T> &a) {  // a * M -> M
  Matrix tempThis(*this);
  m_matF_x_matF(a, tempThis, *this);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::mul(const Matrix<T> &a,
								 const Matrix<T> &b) {  // a * b -> M
  m_matF_x_matF(a, b, *this);
  return (*this);
}

template<class T>
inline Matrix<T> &Matrix<T>::mul(const T a) {
  for (T i = 0; i < 16; i++)
	m[i] *= a;

  return *this;
}

template<class T>
inline Matrix<T> &Matrix<T>::mul(const Matrix &a, const T b) {
  *this = a;
  mul(b);

  return *this;
}

template<class T>
inline void Matrix<T>::mul(Point4<T> &p) const {
  Point4<T> temp;
  mat_x_mat<T>(*this, &p.x, &temp.x);
  p = temp;
}

template<class T>
inline void Matrix<T>::mulP(Point3<T> &p) const {
  // M * p -> d
  Point3<T> d;
  m_mat_x_point3<T>(*this, &p.x, &d.x);
  p = d;
}

template<class T>
inline void Matrix<T>::mulP(const Point3<T> &p, Point3<T> *d) const {
  // M * p -> d
  m_mat_x_point3<T>(*this, &p.x, &d->x);
}

template<class T>
inline void Matrix<T>::mulV(Vector &v) const {
  // M * v -> v
  Vector temp;
  m_mat_x_vector(*this, &v.x, &temp.x);
  v = temp;
}

template<class T>
inline void Matrix<T>::mulV(const Vector &v, Point3<T> *d) const {
  // M * v -> d
  m_mat_x_vector(*this, &v.x, &d->x);
}

template<class T>
inline void Matrix<T>::mul(Box3<T> &b) const {
  m_mat_x_box3<T>(*this, &b.minExtents.x, &b.maxExtents.x);
}

template<class T>
inline Matrix<T> &Matrix<T>::add(const Matrix<T> &a) {
  for (T i = 0; i < 16; ++i)
	m[i] += a.m[i];

  return *this;
}

template<class T>
inline void Matrix<T>::getColumn(unsigned int col, Point4<T> *cptr) const {
  cptr->x = m[col];
  cptr->y = m[col + 4];
  cptr->z = m[col + 8];
  cptr->w = m[col + 12];
}

template<class T>
inline void Matrix<T>::getColumn(unsigned int col, Point3<T> *cptr) const {
  cptr->x = m[col];
  cptr->y = m[col + 4];
  cptr->z = m[col + 8];
}

template<class T>
inline void Matrix<T>::setColumn(unsigned int col, const Point4<T> &cptr) {
  m[col] = cptr.x;
  m[col + 4] = cptr.y;
  m[col + 8] = cptr.z;
  m[col + 12] = cptr.w;
}

template<class T>
inline void Matrix<T>::setColumn(unsigned int col, const Point3<T> &cptr) {
  m[col] = cptr.x;
  m[col + 4] = cptr.y;
  m[col + 8] = cptr.z;
}

template<class T>
inline void Matrix<T>::getRow(unsigned int col, Point4<T> *cptr) const {
  col *= 4;
  cptr->x = m[col++];
  cptr->y = m[col++];
  cptr->z = m[col++];
  cptr->w = m[col];
}

template<class T>
inline void Matrix<T>::getRow(unsigned int col, Point3<T> *cptr) const {
  col *= 4;
  cptr->x = m[col++];
  cptr->y = m[col++];
  cptr->z = m[col];
}

template<class T>
inline void Matrix<T>::setRow(unsigned int col, const Point4<T> &cptr) {
  col *= 4;
  m[col++] = cptr.x;
  m[col++] = cptr.y;
  m[col++] = cptr.z;
  m[col] = cptr.w;
}

template<class T>
inline void Matrix<T>::setRow(unsigned int col, const Point3<T> &cptr) {
  col *= 4;
  m[col++] = cptr.x;
  m[col++] = cptr.y;
  m[col] = cptr.z;
}

template<class T>
inline Point3<T> Matrix<T>::getPosition() const {
  return Point3<T>(m[3], m[3 + 4], m[3 + 8]);
}

template<class T>
inline void Matrix<T>::displace(const Point3<T> &delta) {
  m[3] += delta.x;
  m[3 + 4] += delta.y;
  m[3 + 8] += delta.z;
}

template<class T>
inline Vector Matrix<T>::getForwardVector() const {
  Vector vec;
  getColumn(1, &vec);
  return vec;
}

template<class T>
inline Vector Matrix<T>::getRightVector() const {
  Vector vec;
  getColumn(0, &vec);
  return vec;
}

template<class T>
inline Vector Matrix<T>::getUpVector() const {
  Vector vec;
  getColumn(2, &vec);
  return vec;
}

//------------------------------------
// Math operator overloads
//------------------------------------
template<class T>
inline Matrix<T> Matrix<T>::operator*(const Matrix<T> &m) {
  Matrix<T> tempThis(*this);
  Matrix<T> temp;
  mat_x_mat<T>(tempThis, m, temp);
  return temp;
}

template<class T>
void Matrix<T>::operator=(const glm::mat4x4 m1) {
  memcpy(&m[0], &m1, sizeof(glm::mat4x4));
}

template<class T>
inline Matrix<T> &Matrix<T>::operator*=(const Matrix<T> &m1) {
  Matrix<T> tempThis(*this);
  mat_x_mat<T>(tempThis, m1, *this);
  return (*this);
}

template<class T>
inline Point3<T> Matrix<T>::operator*(const Point3<T> pt) {
  Matrix<T> tempThis(*this);
  Point3<T> d;
  m_mat_x_point3<T>(*this, &pt.x, &d.x);
  return d;
}

template<class T>
inline Point4<T> Matrix<T>::operator*(const Point4<T> pt) {
  Matrix<T> tempThis(*this);
  Point4<T> d;
  m_mat_x_point4<T>(*this, &pt.x, &d.x);
  return d;
}

template<class T>
const Matrix<T> Matrix<T>::Identity(true);

template<class T>
inline void Matrix<T>::transposeTo(T *matrix) const {
  matrix[idx(0, 0)] = m[idx(0, 0)];
  matrix[idx(0, 1)] = m[idx(1, 0)];
  matrix[idx(0, 2)] = m[idx(2, 0)];
  matrix[idx(0, 3)] = m[idx(3, 0)];
  matrix[idx(1, 0)] = m[idx(0, 1)];
  matrix[idx(1, 1)] = m[idx(1, 1)];
  matrix[idx(1, 2)] = m[idx(2, 1)];
  matrix[idx(1, 3)] = m[idx(3, 1)];
  matrix[idx(2, 0)] = m[idx(0, 2)];
  matrix[idx(2, 1)] = m[idx(1, 2)];
  matrix[idx(2, 2)] = m[idx(2, 2)];
  matrix[idx(2, 3)] = m[idx(3, 2)];
  matrix[idx(3, 0)] = m[idx(0, 3)];
  matrix[idx(3, 1)] = m[idx(1, 3)];
  matrix[idx(3, 2)] = m[idx(2, 3)];
  matrix[idx(3, 3)] = m[idx(3, 3)];
}

template<class T>
bool Matrix<T>::isAffine() const {
  // An affine transform is defined by the following structure
  //
  // [ X X X P ]
  // [ X X X P ]
  // [ X X X P ]
  // [ 0 0 0 1 ]
  //
  //  Where X is an orthonormal 3x3 submatrix and P is an arbitrary translation
  //  We'll check in the following order:
  //   1: [3][3] must be 1
  //   2: Shear portion must be zero
  //   3: Dot products of rows and columns must be zero
  //   4: Length of rows and columns must be 1
  //
  if (m[idx(3, 3)]!=1.0f)
	return false;

  if (m[idx(0, 3)]!=0.0f || m[idx(1, 3)]!=0.0f || m[idx(2, 3)]!=0.0f)
	return false;

  Point3<T> one, two, three;
  getColumn(0, &one);
  getColumn(1, &two);
  getColumn(2, &three);
  if (mDot(one, two) > 0.0001f || mDot(one, three) > 0.0001f ||
	  mDot(two, three) > 0.0001f)
	return false;

  if (mFabs(1.0f - one.lenSquared()) > 0.0001f ||
	  mFabs(1.0f - two.lenSquared()) > 0.0001f ||
	  mFabs(1.0f - three.lenSquared()) > 0.0001f)
	return false;

  getRow(0, &one);
  getRow(1, &two);
  getRow(2, &three);
  if (mDot(one, two) > 0.0001f || mDot(one, three) > 0.0001f ||
	  mDot(two, three) > 0.0001f)
	return false;

  if (mFabs(1.0f - one.lenSquared()) > 0.0001f ||
	  mFabs(1.0f - two.lenSquared()) > 0.0001f ||
	  mFabs(1.0f - three.lenSquared()) > 0.0001f)
	return false;

  // We're ok.
  return true;
}

template<class T>
bool Matrix<T>::fullInverse() {
  Point4<T> a, b, c, d;
  getRow(0, &a);
  getRow(1, &b);
  getRow(2, &c);
  getRow(3, &d);

  // det = a0*b1*c2*d3 - a0*b1*c3*d2 - a0*c1*b2*d3 + a0*c1*b3*d2 + a0*d1*b2*c3 -
  // a0*d1*b3*c2 -
  //       b0*a1*c2*d3 + b0*a1*c3*d2 + b0*c1*a2*d3 - b0*c1*a3*d2 - b0*d1*a2*c3 +
  //       b0*d1*a3*c2 + c0*a1*b2*d3 - c0*a1*b3*d2 - c0*b1*a2*d3 + c0*b1*a3*d2 +
  //       c0*d1*a2*b3 - c0*d1*a3*b2 - d0*a1*b2*c3 + d0*a1*b3*c2 + d0*b1*a2*c3 -
  //       d0*b1*a3*c2 - d0*c1*a2*b3 + d0*c1*a3*b2
  T det =
	  a.x*b.y*c.z*d.w - a.x*b.y*c.w*d.z - a.x*c.y*b.z*d.w +
		  a.x*c.y*b.w*d.z + a.x*d.y*b.z*c.w - a.x*d.y*b.w*c.z -
		  b.x*a.y*c.z*d.w + b.x*a.y*c.w*d.z + b.x*c.y*a.z*d.w -
		  b.x*c.y*a.w*d.z - b.x*d.y*a.z*c.w + b.x*d.y*a.w*c.z +
		  c.x*a.y*b.z*d.w - c.x*a.y*b.w*d.z - c.x*b.y*a.z*d.w +
		  c.x*b.y*a.w*d.z + c.x*d.y*a.z*b.w - c.x*d.y*a.w*b.z -
		  d.x*a.y*b.z*c.w + d.x*a.y*b.w*c.z + d.x*b.y*a.z*c.w -
		  d.x*b.y*a.w*c.z - d.x*c.y*a.z*b.w + d.x*c.y*a.w*b.z;

  if (mFabs(det) < 0.00001f)
	return false;

  Point4<T> aa, bb, cc, dd;
  aa.x = b.y*c.z*d.w - b.y*c.w*d.z - c.y*b.z*d.w + c.y*b.w*d.z +
	  d.y*b.z*c.w - d.y*b.w*c.z;
  aa.y = -a.y*c.z*d.w + a.y*c.w*d.z + c.y*a.z*d.w -
	  c.y*a.w*d.z - d.y*a.z*c.w + d.y*a.w*c.z;
  aa.z = a.y*b.z*d.w - a.y*b.w*d.z - b.y*a.z*d.w + b.y*a.w*d.z +
	  d.y*a.z*b.w - d.y*a.w*b.z;
  aa.w = -a.y*b.z*c.w + a.y*b.w*c.z + b.y*a.z*c.w -
	  b.y*a.w*c.z - c.y*a.z*b.w + c.y*a.w*b.z;

  bb.x = -b.x*c.z*d.w + b.x*c.w*d.z + c.x*b.z*d.w -
	  c.x*b.w*d.z - d.x*b.z*c.w + d.x*b.w*c.z;
  bb.y = a.x*c.z*d.w - a.x*c.w*d.z - c.x*a.z*d.w + c.x*a.w*d.z +
	  d.x*a.z*c.w - d.x*a.w*c.z;
  bb.z = -a.x*b.z*d.w + a.x*b.w*d.z + b.x*a.z*d.w -
	  b.x*a.w*d.z - d.x*a.z*b.w + d.x*a.w*b.z;
  bb.w = a.x*b.z*c.w - a.x*b.w*c.z - b.x*a.z*c.w + b.x*a.w*c.z +
	  c.x*a.z*b.w - c.x*a.w*b.z;

  cc.x = b.x*c.y*d.w - b.x*c.w*d.y - c.x*b.y*d.w + c.x*b.w*d.y +
	  d.x*b.y*c.w - d.x*b.w*c.y;
  cc.y = -a.x*c.y*d.w + a.x*c.w*d.y + c.x*a.y*d.w -
	  c.x*a.w*d.y - d.x*a.y*c.w + d.x*a.w*c.y;
  cc.z = a.x*b.y*d.w - a.x*b.w*d.y - b.x*a.y*d.w + b.x*a.w*d.y +
	  d.x*a.y*b.w - d.x*a.w*b.y;
  cc.w = -a.x*b.y*c.w + a.x*b.w*c.y + b.x*a.y*c.w -
	  b.x*a.w*c.y - c.x*a.y*b.w + c.x*a.w*b.y;

  dd.x = -b.x*c.y*d.z + b.x*c.z*d.y + c.x*b.y*d.z -
	  c.x*b.z*d.y - d.x*b.y*c.z + d.x*b.z*c.y;
  dd.y = a.x*c.y*d.z - a.x*c.z*d.y - c.x*a.y*d.z + c.x*a.z*d.y +
	  d.x*a.y*c.z - d.x*a.z*c.y;
  dd.z = -a.x*b.y*d.z + a.x*b.z*d.y + b.x*a.y*d.z -
	  b.x*a.z*d.y - d.x*a.y*b.z + d.x*a.z*b.y;
  dd.w = a.x*b.y*c.z - a.x*b.z*c.y - b.x*a.y*c.z + b.x*a.z*c.y +
	  c.x*a.y*b.z - c.x*a.z*b.y;

  setRow(0, aa);
  setRow(1, bb);
  setRow(2, cc);
  setRow(3, dd);

  mul(1.0f/det);

  return true;
}

template<class T>
EulerF Matrix<T>::toEuler() const {
  const T *mat = m;

  EulerF r;
  r.x = mAsin(mClampF(mat[Matrix<T>::idx(2, 1)], -1.0, 1.0));

  if (mCos(r.x)!=0.f) {
	r.y = mAtan2(-mat[Matrix<T>::idx(2, 0)], mat[Matrix<T>::idx(2, 2)]);
	r.z = mAtan2(-mat[Matrix<T>::idx(0, 1)], mat[Matrix<T>::idx(1, 1)]);
  } else {
	r.y = 0.f;
	r.z = mAtan2(mat[Matrix<T>::idx(1, 0)], mat[Matrix<T>::idx(0, 0)]);
  }

  return r;
}

template<class T>
void Matrix<T>::dumpMatrix(const char *caption) const {
  if (caption)
	std::cout << caption << std::endl;

#define OUT(X)                                                     \
  std::cout << std::fixed << std::setw(11) << std::setprecision(6) \
            << std::setfill(' ') << X;

  OUT(m[idx(0, 0)])
  OUT(m[idx(1, 0)])
  OUT(m[idx(2, 0)])
  OUT(m[idx(3, 0)])

  std::cout << std::endl;

  OUT(m[idx(0, 1)])
  OUT(m[idx(1, 1)])
  OUT(m[idx(2, 1)])
  OUT(m[idx(3, 1)])

  std::cout << std::endl;

  OUT(m[idx(0, 2)])
  OUT(m[idx(1, 2)])
  OUT(m[idx(2, 2)])
  OUT(m[idx(3, 2)])

  std::cout << std::endl;

  OUT(m[idx(0, 3)])
  OUT(m[idx(1, 3)])
  OUT(m[idx(2, 3)])
  OUT(m[idx(3, 3)])

  std::cout << std::endl;
}

//------------------------------------
// Non-member methods
//------------------------------------

template<class T>
inline void mTransformPlane(const Matrix<T> &mat, const Point3<T> &scale,
							const Plane<T> &plane, Plane<T> *result) {
  m_matF_x_scale_x_planeF(mat, &scale.x, &plane.x, &result->x);
}

template<class T>
Matrix<T> mPerspective(float fovy, float aspect, float n, float f) {
  Matrix<T> Perspective;

  float coty = 1.0f/tan(fovy*(float)M_PI/360.0f);

  Perspective.m[0] = coty/aspect;
  Perspective.m[5] = coty;
  Perspective.m[10] = (n + f)/(n - f);
  Perspective.m[11] = -1.0f;
  Perspective.m[14] = 2.0f*n*f/(n - f);
  Perspective.m[15] = 0.0f;

  return Perspective;
}

typedef Matrix<float> MatrixF;

}  // namespace geometry
}  // namespace omega
