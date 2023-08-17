#include <geometry/Quaternion.h>
#include <geometry/Math.h>
#include <geometry/Point3.h>

using namespace omega::geometry;

OQuaternion::OQuaternion(float Angle, const OVector& V)
{
    float HalfAngleInRadians = mRadians(Angle / 2);

    float SineHalfAngle = sinf(HalfAngleInRadians);
    float CosHalfAngle = cosf(HalfAngleInRadians);

    x = V.x * SineHalfAngle;
    y = V.y * SineHalfAngle;
    z = V.z * SineHalfAngle;
    w = CosHalfAngle;
}

OQuaternion::OQuaternion(float _x, float _y, float _z, float _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
}

void OQuaternion::normalize()
{
    float Length = sqrtf(x * x + y * y + z * z + w * w);

    x /= Length;
    y /= Length;
    z /= Length;
    w /= Length;
}


OQuaternion OQuaternion::conjugate() const
{
    OQuaternion ret(-x, -y, -z, w);
    return ret;
}


OQuaternion OQuaternion::operator*( const OVector& v)
{
    float w = -(this->x * v.x) - (this->y * v.y) - (this->z * v.z);
    float x = (this->w * v.x) + (this->y * v.z) - (this->z * v.y);
    float y = (this->w * v.y) + (this->z * v.x) - (this->x * v.z);
    float z = (this->w * v.z) + (this->x * v.y) - (this->y * v.x);

    OQuaternion ret(x, y, z, w);

    return ret;
}


OQuaternion OQuaternion::operator*(const OQuaternion& r)
{
    float w = (this->w * r.w) - (this->x * r.x) - (this->y * r.y) - (this->z * r.z);
    float x = (this->x * r.w) + (this->w * r.x) + (this->y * r.z) - (this->z * r.y);
    float y = (this->y * r.w) + (this->w * r.y) + (this->z * r.x) - (this->x * r.z);
    float z = (this->z * r.w) + (this->w * r.z) + (this->x * r.y) - (this->y * r.x);

    OQuaternion ret(x, y, z, w);

    return ret;
}



OVector OQuaternion::toDegrees()
{
    float f[3];

    f[0] = atan2(x * z + y * w, x * w - y * z);
    f[1] = acos(-x * x - y * y - z * z - w * w);
    f[2] = atan2(x * z - y * w, x * w + y * z);

    f[0] = mDegrees(f[0]);
    f[1] = mDegrees(f[1]);
    f[2] = mDegrees(f[2]);

    return OVector(f);
}
