#include <render/CameraFly.h>

using namespace omega::render;

CameraFly::CameraFly(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Camera(position, up, yaw, pitch) {
}

CameraFly::CameraFly(float posX, float posY, float posZ, float upX, float upY, float upZ,
					 float yaw, float pitch) : Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch) {
}
