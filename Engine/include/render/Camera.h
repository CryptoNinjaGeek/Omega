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
#include <reactphysics3d/reactphysics3d.h>

namespace omega {
namespace render {

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, JUMP };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
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

  glm::mat4 viewMatrix() const;

  virtual void processKeyboard(Camera_Movement direction, float deltaTime);
  void processMouseMovement(float xoffset, float yoffset,
							bool constrainPitch = true);
  void processMouseScroll(float yoffset);
  auto front() const -> glm::vec3 { return front_; }
  auto position() const -> glm::vec3 { return position_; }
  auto setPositon(glm::vec3 pos) -> void { position_ = pos; }
  auto setLookAt(glm::vec3 lookAt) -> void;
  auto setPerspective(float const &fov, float const &width, float const &height,
					  float const &near, float const &far) -> void;
  auto projectionMatrix() const -> glm::mat4x4 { return projection_matrix_; }

  /// Field of view angle in degrees (Y axis) — last value passed to setPerspective().
  auto fov() const -> float { return fov_; }
  /// Near clipping distance — last value passed to setPerspective().
  auto nearPlane() const -> float { return near_plane_; }
  /// Far clipping distance — last value passed to setPerspective().
  auto farPlane() const -> float { return far_plane_; }
  /// Viewport aspect ratio (width/height) — derived from setPerspective().
  auto aspectRatio() const -> float { return aspect_ratio_; }
  glm::mat4x4 calculate_lookAt_matrix(glm::vec3 position, glm::vec3 target,
									  glm::vec3 worldUp);
  void updateCameraVectors();

  glm::vec3 entityPosition() { return position_; }
  glm::vec3 entityDirection() { return front_; }

  auto setupPhysics(reactphysics3d::PhysicsWorld *, reactphysics3d::PhysicsCommon *) -> void;

  /// Horizontal movement speed in world-units per second (used by the base
  /// Camera::processKeyboard, which integrates position_ directly). The default
  /// `SPEED = 2.5` is tuned for small interior scenes; larger demos — terrains,
  /// outdoor walks — will want to raise it so WASD feels like a walk rather
  /// than a crawl. `CameraFPS` ignores this value, it uses `applyWorldForce`.
  auto movementSpeed() const -> float { return movement_speed_; }
  auto setMovementSpeed(float speed) -> void { movement_speed_ = speed; }

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

  reactphysics3d::RigidBody *body_{nullptr};
  reactphysics3d::Collider *collider_{nullptr};

  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 world_up_;

  // Cached perspective parameters (populated by setPerspective). Sensible
  // defaults so that callers inspecting fov()/nearPlane()/farPlane()/aspectRatio()
  // before setPerspective has been called still get reasonable numbers.
  float fov_{ZOOM};
  float near_plane_{0.1f};
  float far_plane_{100.0f};
  float aspect_ratio_{16.0f / 9.0f};
};
}  // namespace render
}  // namespace omega
