#pragma once

#include "render/Camera.h"

namespace omega {
namespace render {

class OMEGA_EXPORT CameraFPS : public Camera {
public:
  // constructor with vectors
  CameraFPS(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
			float pitch = PITCH);

  // constructor with scalar values
  CameraFPS(float posX, float posY, float posZ, float upX, float upY, float upZ,
			float yaw, float pitch);

  virtual void processKeyboard(Camera_Movement direction, float deltaTime);

private:
  float avg_camera_speed{0.f};

};
}  // namespace render
}  // namespace omega
