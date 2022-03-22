//
// Created by Carsten Tang on 20/03/2022.
//

#include "OCamera.h"
#include "OMatrix.h"
#include "OKeyCodes.h"

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;

OCamera::OCamera()
{
    X = OVector(1.0f, 0.0f, 0.0f);
    Y = OVector(0.0f, 1.0f, 0.0f);
    Z = OVector(0.0f, 0.0f, 1.0f);

    Position = OVector(0.0f, 0.0f, 5.0f);
    Reference = OVector(0.0f, 0.0f, 0.0f);

    calculateViewMatrix();
}

OCamera::~OCamera()
{
}

void OCamera::look(const OVector &Position, const OVector &Reference, bool mRotateAroundReference)
{
    this->Position = Position;
    this->Reference = Reference;

    Z = mNormalize(Position - Reference);

    GetXY(Z, X, Y);

    if(!mRotateAroundReference)
    {
        this->Reference = this->Position - Z * 0.05f;
    }

    calculateViewMatrix();
}

void OCamera::move(const OVector &Movement)
{
    Position += Movement;
    Reference += Movement;

    Position.dump("Position");
    Reference.dump("Reference");

    calculateViewMatrix();
}

OVector OCamera::onKeys(char Keys, float FrameTime)
{
    float Speed = 5.0f;

    if(Keys & 0x40) Speed *= 2.0f;
    if(Keys & 0x80) Speed *= 0.5f;

    float Distance = Speed * FrameTime;

    OVector Up(0.0f, 1.0f, 0.0f);
    OVector Right = X;
    OVector Forward = mCross(Up, Right);

    Up *= Distance;
    Right *= Distance;
    Forward *= Distance;

    OVector Movement;

    switch(Keys) {
    case KEY_LEFT: Movement -= Right; break;
    case KEY_RIGHT: Movement += Right; break;
    case KEY_UP: Movement += Forward; break;
    case KEY_DOWN: Movement -= Forward; break;
    }

    return Movement;
}



void OCamera::onMouseMove(int dx, int dy)
{
    float Sensitivity = 0.25f;

    Position -= Reference;

    if(dx != 0)
    {
        float DeltaX = (float)dx * Sensitivity;

        X = mRotate(X, DeltaX, OVector(0.0f, 1.0f, 0.0f));
        Y = mRotate(Y, DeltaX, OVector(0.0f, 1.0f, 0.0f));
        Z = mRotate(Z, DeltaX, OVector(0.0f, 1.0f, 0.0f));
    }

    if(dy != 0)
    {
        float DeltaY = (float)dy * Sensitivity;

        Y = mRotate(Y, DeltaY, X);
        Z = mRotate(Z, DeltaY, X);

        if(Y.y < 0.0f)
        {
            Z = OVector(0.0f, Z.y > 0.0f ? 1.0f : -1.0f, 0.0f);
            Y = mCross(Z, X);
        }
    }

    Position = Reference + Z * Position.len();

    calculateViewMatrix();
}

void OCamera::onMouseWheel(float zDelta)
{
    Position -= Reference;

    if(zDelta < 0 && Position.len() < 500.0f)
    {
        Position += Position * 0.1f;
    }

    if(zDelta > 0 && Position.len() > 0.05f)
    {
        Position -= Position * 0.1f;
    }

    Position += Reference;

    calculateViewMatrix();
}

void OCamera::setPerspective(float fovy, float aspect, float n, float f)
{
    ProjectionMatrix = mPerspective<float>(fovy, aspect, n, f);
    ProjectionMatrixInverse = ProjectionMatrix.inverse();
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    ViewProjectionMatrixInverse = ViewMatrixInverse * ProjectionMatrixInverse;
}

void OCamera::calculateViewMatrix()
{
    ViewMatrix = OMatrix<float>(X.x, Y.x, Z.x, 0.0f, X.y, Y.y, Z.y, 0.0f, X.z, Y.z, Z.z, 0.0f, -mDot(X, Position), -mDot(Y, Position), -mDot(Z, Position), 1.0f);
    ViewMatrixInverse = ViewMatrix.inverse();
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    ViewProjectionMatrixInverse = ViewMatrixInverse * ProjectionMatrixInverse;
}
