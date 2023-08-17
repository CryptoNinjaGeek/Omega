#pragma once

#include "system/Global.h"
#include "render/Material.h"
#include <memory>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace interface {

class OMEGA_EXPORT Entity {
 public:
  Entity() = default;

  virtual glm::vec3 entityPosition() { return glm::vec3(0, 0, 0); }
  virtual glm::vec3 entityDirection() { return glm::vec3(0, 0, 0); }
};
}  // namespace interface
}  // namespace omega
