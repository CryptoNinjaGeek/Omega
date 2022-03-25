#pragma once

#include "OGlobal.h"

namespace omega
{
    namespace geometry 
    {
        template <class T>
        class OPoint3;
        typedef OPoint3<float> OVector;

        class OMEGA_EXPORT OQuaternion
        {
        public:
            float x, y, z, w;

        public:
            OQuaternion(float Angle, const OVector& V);
            OQuaternion(float _x, float _y, float _z, float _w);
            void normalize();
            OQuaternion conjugate() const;
            OVector toDegrees();

            OQuaternion operator*(const OQuaternion& r);
            OQuaternion operator*(const OVector& v);
        };
    }
}


