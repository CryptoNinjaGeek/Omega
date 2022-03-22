#pragma once

#include "OPoint3.h"
#include "OMatrix.h"
#include "OMath.h"

namespace omega {
    namespace geometry {
        template <class T>
        OMatrix<T> mRotate(float angle, const OPoint3<T> &u)
        {
            OMatrix<T> Rotate;
            OPoint3<T> v = u;

            angle = angle / 180.0f * (float)M_PI;
            mNormalize(u);

            float c = 1.0f - cos(angle), s = sin(angle);

            Rotate.m[0] = 1.0f + c * (v.x * v.x - 1.0f);
            Rotate.m[1] = c * v.x * v.y + v.z * s;
            Rotate.m[2] = c * v.x * v.z - v.y * s;
            Rotate.m[4] = c * v.x * v.y - v.z * s;
            Rotate.m[5] = 1.0f + c * (v.y * v.y - 1.0f);
            Rotate.m[6] = c * v.y * v.z + v.x * s;
            Rotate.m[8] = c * v.x * v.z + v.y * s;
            Rotate.m[9] = c * v.y * v.z - v.x * s;
            Rotate.m[10] = 1.0f + c * (v.z * v.z - 1.0f);

            return Rotate;
        }
    }

    using namespace geometry;
    namespace render {
        enum OCamera_Movement {
            FORWARD,
            BACKWARD,
            LEFT,
            RIGHT
        };

        class OCamera
        {
        public:
            OVector X, Y, Z, Position, Reference;

        public:
            OMatrix<float> ViewMatrix, ViewMatrixInverse, ProjectionMatrix, ProjectionMatrixInverse, ViewProjectionMatrix, ViewProjectionMatrixInverse;

        public:
            OCamera();
            ~OCamera();

        public:
            void look(const OVector &Position, const OVector &Reference, bool RotateAroundReference = false);
            void move(const OVector &Movement);
            OVector onKeys(char Keys, float FrameTime);
            void onMouseMove(int dx, int dy);
            void onMouseWheel(float zDelta);
            void setPerspective(float fovy, float aspect, float n, float f);

        private:
            void calculateViewMatrix();
        };
    }
}

