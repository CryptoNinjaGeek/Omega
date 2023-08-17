#pragma once

#include "system/Global.h"
#include "render/Material.h"
#include <interface/Entity.h>
#include <memory>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace render {
class Shader;
}
namespace interface {

enum class LightType { POINT, DIRECTIONAL, SPOT };

class OMEGA_EXPORT Light : public Entity {
 public:
  Light() = default;

  virtual LightType type() = 0;
  virtual void setup(std::shared_ptr<render::Shader>) = 0;
};
}  // namespace interface
}  // namespace omega