#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include <geometry/Point3.h>
#include <geometry/Matrix.h>
#include <geometry/Math.h>
#include <system/Global.h>
#include <render/Shader.h>
#include <interface/Entity.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace omega {
namespace render {
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class OMEGA_EXPORT Camera : public interface::Entity {
 public:
  // constructor with vectors
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
         float pitch = PITCH)
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
  // constructor with scalar values
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
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

  auto setShader(std::shared_ptr<Shader> shader) -> void { shader_ = shader; }

  auto updateShader() -> void {
    if (shader_) {
      shader_->use();

      shader_->setVec3("viewPos", position_);
      shader_->setVec3("viewFront", front_);
    }
  }

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 viewMatrix() {
    /*    return calculate_lookAt_matrix(glm::vec3(position_.x, 0.0f,
       position_.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      */
    return glm::lookAt(position_, position_ + front_, up_);
  }

  // processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = movement_speed_ * deltaTime;
    if (direction == FORWARD) position_ += front_ * velocity;
    if (direction == BACKWARD) position_ -= front_ * velocity;
    if (direction == LEFT) position_ -= right_ * velocity;
    if (direction == RIGHT) position_ += right_ * velocity;
    position_.y = 0;  // Locks to ground
  }

  // processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void processMouseMovement(float xoffset, float yoffset,
                            bool constrainPitch = true) {
    xoffset *= mouse_sensitivity_;
    yoffset *= mouse_sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
      if (pitch_ > 89.0f) pitch_ = 89.0f;
      if (pitch_ < -89.0f) pitch_ = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
  }

  // processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis
  void processMouseScroll(float yoffset) {
    zoom_ -= (float)yoffset;
    if (zoom_ < 1.0f) zoom_ = 1.0f;
    if (zoom_ > 45.0f) zoom_ = 45.0f;
  }

  auto front() -> glm::vec3 { return front_; }

  auto position() -> glm::vec3 { return position_; }

  auto setPositon(glm::vec3 pos) -> void { position_ = pos; }

  auto setLookAt(glm::vec3 lookAt) -> void {
    front_ = glm::normalize(lookAt - position_);
  }

  auto setPerspective(float const& fov, float const& width, float const& height,
                      float const& near, float const& far) -> void {
    projection_matrix_ = glm::perspectiveFov(fov, width, height, near, far);
  }

  auto projectionMatrix() -> glm::mat4x4 { return projection_matrix_; }

  glm::mat4x4 calculate_lookAt_matrix(glm::vec3 position, glm::vec3 target,
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
    return rotation * translation;  // Remember to read from right to left
                                    // (first translation then rotation)
  }

  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front);
    // also re-calculate the Right and Up vector
    right_ = glm::normalize(glm::cross(
        front_, world_up_));  // normalize the vectors, because their length
                              // gets closer to 0 the more you look up or down
                              // which results in slower movement.
    up_ = glm::normalize(glm::cross(right_, front_));
  }

  // Entity functions
  glm::vec3 entityPosition() { return position_; }
  glm::vec3 entityDirection() { return front_; }

 private:
  // camera Attributes
  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 world_up_;

  glm::mat4x4 projection_matrix_;

  std::shared_ptr<Shader> shader_;

  // euler Angles
  float yaw_;
  float pitch_;

  // camera options
  float movement_speed_;
  float mouse_sensitivity_;
  float zoom_;
};
}  // namespace render
}  // namespace omega
