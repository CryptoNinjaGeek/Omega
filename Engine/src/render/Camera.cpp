#include <render/Camera.h>

#include "glm/ext.hpp"
#include <reactphysics3d/reactphysics3d.h>

using namespace omega::render;

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	: front_(glm::vec3(0.0f, 0.0f, -1.0f)),
	  movement_speed_(SPEED),
	  mouse_sensitivity_(SENSITIVITY),
	  zoom_(ZOOM) {
  position_ = position;
  world_up_ = up;
  yaw_ = yaw;
  pitch_ = pitch;

  updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
			   float yaw, float pitch)
	: front_(glm::vec3(0.0f, 0.0f, -1.0f)),
	  movement_speed_(SPEED),
	  mouse_sensitivity_(SENSITIVITY),
	  zoom_(ZOOM) {
  position_ = glm::vec3(posX, posY, posZ);
  world_up_ = glm::vec3(upX, upY, upZ);
  yaw_ = yaw;
  pitch_ = pitch;
  updateCameraVectors();
}

auto Camera::updateShader() -> void {
  if (body_) {
	auto transform = body_->getTransform();
	position_ = glm::vec3(transform.getPosition().x, transform.getPosition().y,
						  transform.getPosition().z);
  }
  if (shader_) {
	shader_->use();

	shader_->setVec3("viewPos", position_);
	shader_->setVec3("viewFront", front_);
  }
}

glm::mat4 Camera::viewMatrix() {
  return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = movement_speed_*deltaTime;
  if (direction==FORWARD)
	position_ += front_*velocity;
  if (direction==BACKWARD)
	position_ -= front_*velocity;
  if (direction==LEFT)
	position_ -= right_*velocity;
  if (direction==RIGHT)
	position_ += right_*velocity;
  // position_.y = 0;  // Locks to ground
}

void Camera::processMouseMovement(float xoffset, float yoffset,
								  bool constrainPitch) {
  xoffset *= mouse_sensitivity_;
  yoffset *= mouse_sensitivity_;

  yaw_ += xoffset;
  pitch_ += yoffset;

  // make sure that when pitch is out of bounds, screen doesn't get flipped
  if (constrainPitch) {
	if (pitch_ > 89.0f)
	  pitch_ = 89.0f;
	if (pitch_ < -89.0f)
	  pitch_ = -89.0f;
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
  zoom_ -= (float)yoffset;
  if (zoom_ < 1.0f)
	zoom_ = 1.0f;
  if (zoom_ > 45.0f)
	zoom_ = 45.0f;
}

auto Camera::setLookAt(glm::vec3 lookAt) -> void {
  front_ = glm::normalize(lookAt - position_);
}

auto Camera::setPerspective(float const &fov, float const &width, float const &height,
							float const &near, float const &far) -> void {
  projection_matrix_ = glm::perspectiveFov(fov, width, height, near, far);
}

glm::mat4x4 Camera::calculate_lookAt_matrix(glm::vec3 position, glm::vec3 target,
											glm::vec3 worldUp) {
  // 1. Position = known
  // 2. Calculate cameraDirection
  glm::vec3 zaxis = glm::normalize(position - target);
  // 3. Get positive right axis vector
  glm::vec3 xaxis =
	  glm::normalize(glm::cross(glm::normalize(worldUp), zaxis));
  // 4. Calculate camera up vector
  glm::vec3 yaxis = glm::cross(zaxis, xaxis);

  // Create translation and rotation matrix
  // In glm we access elements as mat[col][row] due to column-major layout
  glm::mat4 translation = glm::mat4(1.0f);  // Identity matrix by default
  translation[3][0] = -position.x;          // Third column, first row
  translation[3][1] = -position.y;
  translation[3][2] = -position.z;
  glm::mat4 rotation = glm::mat4(1.0f);
  rotation[0][0] = xaxis.x;  // First column, first row
  rotation[1][0] = xaxis.y;
  rotation[2][0] = xaxis.z;
  rotation[0][1] = yaxis.x;  // First column, second row
  rotation[1][1] = yaxis.y;
  rotation[2][1] = yaxis.z;
  rotation[0][2] = zaxis.x;  // First column, third row
  rotation[1][2] = zaxis.y;
  rotation[2][2] = zaxis.z;

  // Return lookAt matrix as combination of translation and rotation matrix
  return rotation*translation;  // Remember to read from right to left
  // (first translation then rotation)
}

// calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors() {
  // calculate the new Front vector
  glm::vec3 front;
  front.x = cos(glm::radians(yaw_))*cos(glm::radians(pitch_));
  front.y = sin(glm::radians(pitch_));
  front.z = sin(glm::radians(yaw_))*cos(glm::radians(pitch_));
  front_ = glm::normalize(front);
  // also re-calculate the Right and Up vector
  right_ = glm::normalize(glm::cross(
	  front_, world_up_));  // normalize the vectors, because their length
  // gets closer to 0 the more you look up or down
  // which results in slower movement.
  up_ = glm::normalize(glm::cross(right_, front_));
}

auto Camera::setupPhysics(reactphysics3d::PhysicsWorld *world,
						  reactphysics3d::PhysicsCommon *physicsCommon) -> void {

  auto height = position_.y*1.1f;
  auto eyeAdjust = position_.y - (height/2.f);

  reactphysics3d::Transform transform = reactphysics3d::Transform::identity();
  transform.setPosition(reactphysics3d::Vector3(position_.x, eyeAdjust, position_.z));

  body_ = world->createRigidBody(transform);
  body_->setType(reactphysics3d::BodyType::DYNAMIC);
  body_->setAngularDamping(0.3);
  body_->setLinearDamping(1.0f);

  transform = reactphysics3d::Transform::identity();
  auto shape = physicsCommon
	  ->createSphereShape(height/2.f);
  collider_ = body_->addCollider(shape, transform);

  reactphysics3d::Material &material = collider_->getMaterial();
  material.setBounciness(0.f);
}

