#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include <geometry/Math.h>
#include <system/Global.h>
#include <render/Shader.h>
#include "geometry/Entity.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace omega {
namespace render {

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, JUMP };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 3.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class OMEGA_EXPORT Camera : public interface::Entity {
public:
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		 glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
		 float pitch = PITCH);

  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
		 float yaw, float pitch);

  auto setShader(std::shared_ptr<Shader> shader) -> void { shader_ = shader; }

  auto updateShader() -> void;

  glm::mat4 viewMatrix();

  virtual void processKeyboard(Camera_Movement direction, float deltaTime);
  void processMouseMovement(float xoffset, float yoffset,
							bool constrainPitch = true);
  void processMouseScroll(float yoffset);
  auto front() -> glm::vec3 { return front_; }
  auto position() -> glm::vec3 { return position_; }
  auto setPositon(glm::vec3 pos) -> void { position_ = pos; }
  auto setLookAt(glm::vec3 lookAt) -> void;
  auto setPerspective(float const &fov, float const &width, float const &height,
					  float const &near, float const &far) -> void;
  auto projectionMatrix() -> glm::mat4x4 { return projection_matrix_; }
  glm::mat4x4 calculate_lookAt_matrix(glm::vec3 position, glm::vec3 target,
									  glm::vec3 worldUp);
  void updateCameraVectors();

  auto entityPosition() -> glm::vec3 override { return position_; }
  auto entityDirection() -> glm::vec3 override { return front_; }
  auto entityModel() -> glm::mat4 override;

  auto setupPhysics(glm::mat4 mat = glm::mat4(1.0f)) -> void;

protected:
  // camera Attributes
  glm::mat4x4 projection_matrix_;

  std::shared_ptr<Shader> shader_;

  // euler Angles
  float yaw_;
  float pitch_;

  // camera options
  float movement_speed_;
  float mouse_sensitivity_;
  float zoom_;
  float eye_adjustment_{0.0f};

  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 world_up_;

};
}  // namespace render
}  // namespace omega
