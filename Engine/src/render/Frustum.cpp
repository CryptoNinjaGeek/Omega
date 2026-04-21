#include <render/Frustum.h>

#include <cmath>

using namespace omega::render;

namespace {

// Normalise a plane so plane.xyz has unit length, preserving the
// `dot(normal, X) + d >= 0` convention. The Gribb–Hartmann extraction
// produces planes with arbitrary scale; normalising lets callers treat
// the w component as a signed distance in world units.
inline glm::vec4 normalisePlane(const glm::vec4 &plane) {
  const float len =
      std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
  if (len <= 0.0f) {
    return plane;  // Degenerate — caller should never see this in practice.
  }
  return plane / len;
}

}  // namespace

std::array<glm::vec4, Frustum::COUNT>
Frustum::extractPlanes(const glm::mat4 &m) {
  // glm matrices are column-major: m[col][row]. The classic Gribb–Hartmann
  // formulas are expressed row-wise, so rowN(i) = m[i][N]. We capture each
  // row once for readability.
  const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
  const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
  const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
  const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

  std::array<glm::vec4, COUNT> planes{};
  planes[LEFT]   = normalisePlane(row3 + row0);
  planes[RIGHT]  = normalisePlane(row3 - row0);
  planes[BOTTOM] = normalisePlane(row3 + row1);
  planes[TOP]    = normalisePlane(row3 - row1);
  planes[NEAR]   = normalisePlane(row3 + row2);
  planes[FAR]    = normalisePlane(row3 - row2);
  return planes;
}

bool Frustum::allPointsOutsideAnyPlane(
    const std::array<glm::vec4, COUNT> &planes,
    const glm::vec3 *points, std::size_t count) {
  if (!points || count == 0) return true;
  for (const auto &plane : planes) {
    bool allOutside = true;
    for (std::size_t i = 0; i < count; ++i) {
      const float d =
          plane.x * points[i].x + plane.y * points[i].y +
          plane.z * points[i].z + plane.w;
      if (d >= 0.0f) {
        allOutside = false;
        break;
      }
    }
    if (allOutside) return true;
  }
  return false;
}

bool Frustum::anyPointInside(
    const std::array<glm::vec4, COUNT> &planes,
    const glm::vec3 *points, std::size_t count) {
  if (!points || count == 0) return false;
  for (std::size_t i = 0; i < count; ++i) {
    if (containsPoint(planes, points[i])) return true;
  }
  return false;
}

bool Frustum::containsPoint(const std::array<glm::vec4, COUNT> &planes,
                            const glm::vec3 &point) {
  for (const auto &plane : planes) {
    const float d = plane.x * point.x + plane.y * point.y +
                    plane.z * point.z + plane.w;
    if (d < 0.0f) return false;
  }
  return true;
}

bool Frustum::sphereOutsideAnyPlane(
    const std::array<glm::vec4, COUNT> &planes,
    const glm::vec3 &center, float radius) {
  // A negative radius is nonsensical but clamp to 0 rather than silently
  // inverting the meaning of the test.
  const float r = radius < 0.0f ? 0.0f : radius;
  for (const auto &plane : planes) {
    const float signedDist = plane.x * center.x + plane.y * center.y +
                             plane.z * center.z + plane.w;
    if (signedDist < -r) {
      return true;  // Sphere entirely on the outside of this plane.
    }
  }
  return false;
}
