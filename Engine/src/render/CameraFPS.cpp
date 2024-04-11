#include <render/CameraFPS.h>
#include <reactphysics3d/reactphysics3d.h>

using namespace omega::render;

CameraFPS::CameraFPS(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Camera(position, up, yaw, pitch) {
}

CameraFPS::CameraFPS(float posX, float posY, float posZ, float upX, float upY, float upZ,
					 float yaw, float pitch) : Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch) {
}

void CameraFPS::processKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = movement_speed_*deltaTime;
  auto world_position = collider_->getLocalToWorldTransform().getPosition();
  position_ = glm::vec3(world_position.x, world_position.y, world_position.z);
  reactphysics3d::Vector3 camVelocity = body_->getLinearVelocity();
  float camSpeed = camVelocity.y;

  avg_camera_speed = avg_camera_speed*0.9f + camSpeed*0.1f;
  float latteralSpeed = reactphysics3d::Vector2(camVelocity.x, camVelocity.z).length();

  if (direction==JUMP && abs(camSpeed) < 1) {
	body_->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(0, 1, 0)*300);
  }
  if (direction==FORWARD && latteralSpeed < 5) {
	body_->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(front_.x, front_.y, front_.z)*20);
  }
  if (direction==BACKWARD && latteralSpeed < 5) {
	body_->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(front_.x, front_.y, front_.z)*-20);
  }
  if (direction==LEFT) {
	body_->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(-1*right_.x, -1*right_.y, -1*right_.z)*20);
  }
  if (direction==RIGHT) {
	body_->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(right_.x, right_.y, right_.z)*20);
  }
}
