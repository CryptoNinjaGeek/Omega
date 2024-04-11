#pragma once

#include <geometry/Math.h>
#include <geometry/Point3.h>
#include <geometry/Point2.h>
#include <geometry/Matrix.h>
#include <geometry/Sphere.h>
#include <geometry/BoxBase.h>

namespace omega {
namespace geometry {
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec3 tangent;
  glm::vec3 bitangent;
};

}  // namespace geometry
}  // namespace omega