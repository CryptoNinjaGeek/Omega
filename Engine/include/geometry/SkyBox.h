#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <geometry/Object.h>
#include <interface/Entity.h>
#include <interface/Light.h>
#include <memory>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

namespace omega {
namespace geometry {

class OMEGA_EXPORT SkyBox : public Object {
 public:
  SkyBox(unsigned int vao, unsigned int vbo, unsigned int cnt);

  virtual void render(std::shared_ptr<render::Camera>);
};
}  // namespace geometry
}  // namespace omega
