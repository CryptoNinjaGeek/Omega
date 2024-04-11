#pragma once

#include "render/Camera.h"

namespace omega {
namespace render {

class OMEGA_EXPORT CameraFly : public Camera {
public:
  // constructor with vectors
  CameraFly(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
			float pitch = PITCH);

  // constructor with scalar values
  CameraFly(float posX, float posY, float posZ, float upX, float upY, float upZ,
			float yaw, float pitch);

};
}  // namespace render
}  // namespace omega
