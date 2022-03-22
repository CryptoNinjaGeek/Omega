#pragma once

#include <math.h>

#define POINT_EPSILON (0.0001f) ///< Epsilon for point types.
#define PARALLEL_PLANE  1e20f

template <class T>
T mRadians(T degree)
{
    T pi = 3.14159265359;
    return (degree * (pi / 180));
}

template <class T>
inline double mSqrt(const T val)
{
    return (T) sqrt(val);
}

template <class T>
inline static void m_point3_normalize(T *p)
{
    T factor = 1.0f / mSqrt<T>(p[0]*p[0] + p[1]*p[1] + p[2]*p[2] );
    p[0] *= factor;
    p[1] *= factor;
    p[2] *= factor;
}


inline static void m_point3D_interpolate(const double *from, const double *to, double factor, double *result )
{
    double inverse = 1.0f - factor;
    result[0] = from[0] * inverse + to[0] * factor;
    result[1] = from[1] * inverse + to[1] * factor;
    result[2] = from[2] * inverse + to[2] * factor;
}


inline static void m_OPoint2_normalize(double *p)
{
    double factor = 1.0f / mSqrt(p[0]*p[0] + p[1]*p[1] );
    p[0] *= factor;
    p[1] *= factor;
}

inline static void m_OPoint2_normalize_f(double *p, double val)
{
    double factor = val / mSqrt(p[0]*p[0] + p[1]*p[1] );
    p[0] *= factor;
    p[1] *= factor;
}

inline double mFabs(const double val)
{
    return fabs(val);
}

template <class T>
inline T mCos(const T angle)
{
    return (T) cos(angle);
}

template <class T>
void mat_x_mat(const T *a, const T *b, T *mresult)
{
    mresult[0] = a[0]*b[0] + a[1]*b[4] + a[2]*b[8]  + a[3]*b[12];
    mresult[1] = a[0]*b[1] + a[1]*b[5] + a[2]*b[9]  + a[3]*b[13];
    mresult[2] = a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14];
    mresult[3] = a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15];

    mresult[4] = a[4]*b[0] + a[5]*b[4] + a[6]*b[8]  + a[7]*b[12];
    mresult[5] = a[4]*b[1] + a[5]*b[5] + a[6]*b[9]  + a[7]*b[13];
    mresult[6] = a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14];
    mresult[7] = a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15];

    mresult[8] = a[8]*b[0] + a[9]*b[4] + a[10]*b[8] + a[11]*b[12];
    mresult[9] = a[8]*b[1] + a[9]*b[5] + a[10]*b[9] + a[11]*b[13];
    mresult[10]= a[8]*b[2] + a[9]*b[6] + a[10]*b[10]+ a[11]*b[14];
    mresult[11]= a[8]*b[3] + a[9]*b[7] + a[10]*b[11]+ a[11]*b[15];

    mresult[12]= a[12]*b[0]+ a[13]*b[4]+ a[14]*b[8] + a[15]*b[12];
    mresult[13]= a[12]*b[1]+ a[13]*b[5]+ a[14]*b[9] + a[15]*b[13];
    mresult[14]= a[12]*b[2]+ a[13]*b[6]+ a[14]*b[10]+ a[15]*b[14];
    mresult[15]= a[12]*b[3]+ a[13]*b[7]+ a[14]*b[11]+ a[15]*b[15];
}

template <class T>
inline void m_mat_x_point3(const T *m, const T *p, T *presult)
{
    presult[0] = m[0]*p[0] + m[1]*p[1] + m[2]*p[2]  + m[3];
    presult[1] = m[4]*p[0] + m[5]*p[1] + m[6]*p[2]  + m[7];
    presult[2] = m[8]*p[0] + m[9]*p[1] + m[10]*p[2] + m[11];
}

template <class T>
inline void m_mat_x_vector(const T *m, const T *v, T *vresult)
{
    vresult[0] = m[0]*v[0] + m[1]*v[1] + m[2]*v[2];
    vresult[1] = m[4]*v[0] + m[5]*v[1] + m[6]*v[2];
    vresult[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2];
}


template <class T>
static void m_mat_x_box3(const T *m, T* min, T* max)
{
    T originalMin[3];
    T originalMax[3];
    originalMin[0] = min[0];
    originalMin[1] = min[1];
    originalMin[2] = min[2];
    originalMax[0] = max[0];
    originalMax[1] = max[1];
    originalMax[2] = max[2];

    min[0] = max[0] = m[3];
    min[1] = max[1] = m[7];
    min[2] = max[2] = m[11];

    const T * row = &m[0];
    for (unsigned int i = 0; i < 3; i++)
    {
#define  Do_One_Row(j)   {                         \
         T    a = (row[j] * originalMin[j]);           \
         T    b = (row[j] * originalMax[j]);           \
         if (a < b) { *min += a;  *max += b; }           \
         else       { *min += b;  *max += a; }     }

        // Simpler addressing (avoiding things like [ecx+edi*4]) might be worthwhile (LH):
        Do_One_Row(0);
        Do_One_Row(1);
        Do_One_Row(2);
        row += 4;
        min++;
        max++;
    }
}

template <class T>
static T m_mat_determinant(const T *m)
{
    return m[0] * (m[5] * m[10] - m[6] * m[9])  +
           m[4] * (m[2] * m[9]  - m[1] * m[10]) +
           m[8] * (m[1] * m[6]  - m[2] * m[5])  ;
}


template <class T>
static void m_mat_inverse(T *m)
{
    T det = m_mat_determinant<T>( m );

    T invDet = 1.0f/det;
    T temp[16];

    temp[0] = (m[5] * m[10]- m[6] * m[9]) * invDet;
    temp[1] = (m[9] * m[2] - m[10]* m[1]) * invDet;
    temp[2] = (m[1] * m[6] - m[2] * m[5]) * invDet;

    temp[4] = (m[6] * m[8] - m[4] * m[10])* invDet;
    temp[5] = (m[10]* m[0] - m[8] * m[2]) * invDet;
    temp[6] = (m[2] * m[4] - m[0] * m[6]) * invDet;

    temp[8] = (m[4] * m[9] - m[5] * m[8]) * invDet;
    temp[9] = (m[8] * m[1] - m[9] * m[0]) * invDet;
    temp[10]= (m[0] * m[5] - m[1] * m[4]) * invDet;

    m[0] = temp[0];
    m[1] = temp[1];
    m[2] = temp[2];

    m[4] = temp[4];
    m[5] = temp[5];
    m[6] = temp[6];

    m[8] = temp[8];
    m[9] = temp[9];
    m[10] = temp[10];

    // invert the translation
    temp[0] = -m[3];
    temp[1] = -m[7];
    temp[2] = -m[11];
    m_mat_x_vector(m, temp, &temp[4]);
    m[3] = temp[4];
    m[7] = temp[5];
    m[11]= temp[6];
}
