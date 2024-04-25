#pragma once

#include <string>
#include <vector>
#include <set>

#include "assimp/matrix4x4.h"
#include "assimp/matrix3x3.h"
#include "glm/ext.hpp"

#include <geometry/Vertex.h>

namespace omega {
namespace utils {

inline std::vector<std::string> split(std::string_view str, std::set<char> delimers) {
  std::vector<std::string> result;
  auto left = str.begin();
  for (auto it = left; it!=str.end(); ++it) {
	if (delimers.contains(*it)) {
	  result.emplace_back(&*left, it - left);
	  left = it + 1;
	}
  }
  if (left!=str.end())
	result.emplace_back(&*left, str.end() - left);
  return result;
}

inline geometry::BoundingBox bounding_box(std::vector<omega::geometry::Vertex> const &points) {
  glm::vec3 min = points[0].position;
  glm::vec3 max = points[0].position;
  for (auto &point : points) {
	min = glm::min(min, point.position);
	max = glm::max(max, point.position);
  }
  auto box = geometry::BoundingBox(min, max);
  return box;
}

inline glm::mat4 convertMatrix(const aiMatrix3x3 &aiMat) {
  return {
	  aiMat.a1, aiMat.b1, aiMat.c1, 0.0f,
	  aiMat.a2, aiMat.b2, aiMat.c2, 0.0f,
	  aiMat.a3, aiMat.b3, aiMat.c3, 0.0f,
	  0.0f, 0.0f, 0.0f, 1.0f
  };
}

inline glm::mat4 convertMatrix(const aiMatrix4x4 &aiMat) {
  return {
	  aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
	  aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
	  aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
	  aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
  };
}

}  // namespace utils
}  // namespace omega
