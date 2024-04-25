#include <render/CameraFPS.h>
#include "physics/PhysicsEngine.h"

using namespace omega::render;

CameraFPS::CameraFPS(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Camera(position, up, yaw, pitch) {
}

CameraFPS::CameraFPS(float posX, float posY, float posZ, float upX, float upY, float upZ,
					 float yaw, float pitch) : Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch) {
}

void CameraFPS::processKeyboard(Camera_Movement direction, float deltaTime) {
  if (!physicsObject_) {
	Camera::processKeyboard(direction, deltaTime);
	return;
  }

  float velocity = movement_speed_*deltaTime;
  auto camVelocity = physicsObject_->velocity();
  float camSpeed = camVelocity.y;

  avg_camera_speed = avg_camera_speed*0.9f + camSpeed*0.1f;
  float latteralSpeed = glm::length(glm::vec2(camVelocity.x, camVelocity.z));

  if (direction==JUMP && abs(camSpeed) < 1) {
	physicsObject_->applyForce(glm::vec3(0, 1, 0)*SPEED*velocity);
  }
  if (direction==FORWARD && latteralSpeed < 5) {
	physicsObject_->move(glm::vec3(front_.x, 0, front_.z)*SPEED*velocity);
  }
  if (direction==BACKWARD && latteralSpeed < 5) {
	physicsObject_->move(glm::vec3(front_.x, 0, front_.z)*-1.f*SPEED*velocity);
  }
  if (direction==LEFT) {
	physicsObject_->move(glm::vec3(-1*right_.x, 0, -1*right_.z)*SPEED*velocity);
  }
  if (direction==RIGHT) {
	physicsObject_->move(glm::vec3(right_.x, 0, right_.z)*SPEED*velocity);
  }
}
