#pragma once

#include<iostream>
#include<fstream>
#include<string>

#include "OPoint3.h"
#include "OMatrix.h"
#include "OMath.h"
#include "OGlobal.h"

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

        class OMEGA_EXPORT OCamera
        {
        public:
            OVector Position, Reference, Up, Movement;
            float yaw = 90.0f;
            float pitch = 0.0f;
            OPoint2<int> m_mousePos;

        public:
            OMatrix<float> ViewMatrix, ViewMatrixInverse, ProjectionMatrix, ProjectionMatrixInverse, ViewProjectionMatrix, ViewProjectionMatrixInverse;

        public:
            OCamera();
            ~OCamera();

        public:
            void look( OVector Position,  OVector Reference, OVector Up = OVector( 0.f , 1.f , 0.f ));
            void move( OVector Movement);
            void move();
            void process(float);
            void stopMovement();
            void onMouseMove(int dx, int dy);
            void onMouseWheel(float zDelta);
            void setPerspective(float fovy, float aspect, float n, float f);
            void setWindowSize(int width, int height);

        private:
            void calculateViewMatrix();
            void processMouseMove();
            void calculateRotations();

        private:
            int m_windowWidth = 1024;
            int m_windowHeight = 768;

            bool m_OnUpperEdge = false;
            bool m_OnLowerEdge = false;
            bool m_OnLeftEdge = false;
            bool m_OnRightEdge = false;
        };

        typedef std::shared_ptr<OCamera> OCameraPtr;
    }
}

