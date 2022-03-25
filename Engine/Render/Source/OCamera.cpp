//
// Created by Carsten Tang on 20/03/2022.
//

#include "OCamera.h"
#include "OMatrix.h"
#include "OKeyCodes.h"
#include "OWindow.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/mat4x4.hpp"

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;

static int MARGIN = 10;
static float EDGE_STEP = 1.0f;


OCamera::OCamera()
{
    look(OVector(0.0f, 0.0f, 5.0f), OVector(0.0f, 0.0f, 0.0f), OVector(0.f, 1.f, 0.f));

    m_OnUpperEdge = false;
    m_OnLowerEdge = false;
    m_OnLeftEdge = false;
    m_OnRightEdge = false;
}

OCamera::~OCamera()
{
}

void OCamera::setWindowSize( int width , int height )
{
    m_windowWidth = width;
    m_windowHeight = height;

    m_mousePos.x = m_windowWidth / 2;
    m_mousePos.y = m_windowHeight / 2;
}

void OCamera::look( OVector newPos,  OVector newRef, OVector newUp)
{
    Up = newUp;
    Position = newPos;
    Reference = newRef;

    OVector HTarget = newRef - newPos;
    HTarget.y = 0;
    HTarget.normalize();   

    float Angle = mDegrees(asin(abs(HTarget.z)));

    if (HTarget.z >= 0.0f)
        if (HTarget.x >= 0.0f)
            yaw = 360.0f - Angle;
        else
            yaw = 180.0f + Angle;
    else
        if (HTarget.x >= 0.0f)
            yaw = Angle;
        else
            yaw = 180.0f - Angle;

    pitch = -mDegrees(asin(Reference.y));
    std::cout << "Yaw=> " << yaw << "  Pitch=> " << pitch << std::endl;

    calculateViewMatrix();
}

void OCamera::move( OVector Movement)
{
    Position += Movement;
    Reference += Movement;

    calculateViewMatrix();
}

void OCamera::move()
{
    if (Movement.len() > 0.0f)
        move(Movement);
}

void OCamera::processMouseMove()
{
    bool ShouldUpdate = false;
    if (m_OnLeftEdge) {
        yaw -= EDGE_STEP;
        ShouldUpdate = true;
    }
    else if (m_OnRightEdge) {
        yaw += EDGE_STEP;
        ShouldUpdate = true;
    }

    if (m_OnUpperEdge) {
        if (pitch > -90.0f) {
            pitch -= EDGE_STEP;
            ShouldUpdate = true;
        }
    }
    else if (m_OnLowerEdge) {
        if (pitch < 90.0f) {
            pitch += EDGE_STEP;
            ShouldUpdate = true;
        }
    }
    if (ShouldUpdate) {
        calculateRotations();
        calculateViewMatrix();
    }
}

void OCamera::process(float deltaTime)
{
    processMouseMove();

    float Speed = 15.0f;
    float Distance = Speed * deltaTime;

    OVector Forward = (Reference - Position);

    Forward.normalize();

    OVector Right = mCross(Forward, Up);
    OVector Vertical = Up * Distance;

    Right *= Distance;
    Forward *= Distance;

    Movement.zero();

    if (OWindow::instance()->isKeyPressed(KEY_A))
        Movement -= Right;
    if (OWindow::instance()->isKeyPressed(KEY_D))
        Movement += Right;
    if (OWindow::instance()->isKeyPressed(KEY_W))
        Movement += Forward;
    if (OWindow::instance()->isKeyPressed(KEY_S))
        Movement -= Forward;
    if (OWindow::instance()->isKeyPressed(KEY_Q))
        Movement -= Vertical;
    if (OWindow::instance()->isKeyPressed(KEY_E))
        Movement += Vertical;

    if (Movement.len() > 0.0f)
        move(Movement);
}

void OCamera::stopMovement() {
    Movement.zero();
}

void OCamera::onMouseMove(int x, int y)
{
    int DeltaX = x - m_mousePos.x;
    int DeltaY = y - m_mousePos.y;

    m_mousePos.x = x;
    m_mousePos.y = y;

    std::cout << "DeltaX => " << DeltaX << "  DeltaY " << DeltaY << std::endl;

    yaw += (float)DeltaX / 20.0f;
    pitch += (float)DeltaY / 20.0f;

    if (DeltaX == 0) {
        if (x <= MARGIN) {
            m_OnLeftEdge = true;
        }
        else if (x >= (m_windowWidth - MARGIN)) {
            m_OnRightEdge = true;
        }
    }
    else {
        m_OnLeftEdge = false;
        m_OnRightEdge = false;
    }

    if (DeltaY == 0) {
        if (y <= MARGIN) {
            m_OnUpperEdge = true;
        }
        else if (y >= (m_windowHeight - MARGIN)) {
            m_OnLowerEdge = true;
        }
    }
    else {
        m_OnUpperEdge = false;
        m_OnLowerEdge = false;
    }

    calculateRotations();
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
    ProjectionMatrix = glm::perspective(glm::radians(fovy), aspect, n, f);
}

void OCamera::calculateRotations()
{
    OVector Yaxis(0.0f, 1.0f, 0.0f);

    // Rotate the view vector by the horizontal angle around the vertical axis
    OVector View(1.0f, 0.0f, 0.0f);
    View.rotate(yaw, Yaxis);
    View.normalize();

    // Rotate the view vector by the vertical angle around the horizontal axis
    OVector U = Yaxis.cross(View);
    U.normalize();
    View.rotate(pitch, U);

    Reference = View;
    Reference.normalize();

    Up = Reference.cross(U);
    Up.normalize();


    Position.dump("Position");
    View.dump("View");

    std::cout << "Yaw=> " << yaw << "  Pitch=> " << pitch << std::endl;
}

void OCamera::calculateViewMatrix()
{
    ViewMatrix = glm::lookAt(
        Position.toVec3(), 
        Reference.toVec3(),
        Up.toVec3()
    );

    ViewMatrixInverse = ViewMatrix;
    ViewMatrixInverse.inverse();

    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    ViewProjectionMatrixInverse = ViewMatrixInverse * ProjectionMatrixInverse;
}
