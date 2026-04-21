#pragma once

#include <system/Global.h>
#include <glm/glm.hpp>
#include <array>

namespace omega {
namespace render {

/**
 * View-frustum plane extraction + point/polygon containment tests.
 *
 * Implements the Gribb–Hartmann method (from "Fast Extraction of Viewing
 * Frustum Planes from the World-View-Projection Matrix", Gil Gribb & Klaus
 * Hartmann 2001): each of the six clipping planes is obtained by adding or
 * subtracting one of the four rows of a combined matrix.
 *
 * Plane encoding (matches the classic form `A*x + B*y + C*z + D = 0`):
 *   plane.xyz = outward-pointing plane normal (points into the frustum)
 *   plane.w   = distance component D
 * A point `P` is *inside* the frustum iff
 *   `dot(plane.xyz, P) + plane.w >= 0`  for every one of the six planes.
 *
 * We expect callers to pass `projection * view` (i.e. world-space points are
 * tested directly). The planes are normalised so that plane.xyz is unit
 * length, which lets callers reason about distances geometrically.
 */
class OMEGA_EXPORT Frustum {
public:
  /// Plane indices in the returned array.
  enum Plane : unsigned {
    LEFT = 0,
    RIGHT = 1,
    BOTTOM = 2,
    TOP = 3,
    NEAR = 4,
    FAR = 5,
    COUNT = 6,
  };

  /// Extract the six world-space frustum planes from a view-projection
  /// matrix. Returned planes are normalised and follow the convention that
  /// `dot(plane.xyz, worldPos) + plane.w >= 0` holds for points inside.
  static std::array<glm::vec4, COUNT>
  extractPlanes(const glm::mat4 &viewProjection);

  /// Fast "outside-only" rejection test for a convex point set. Returns true
  /// iff every one of the supplied points lies on the negative side of the
  /// same plane — in which case the convex hull of the points cannot
  /// intersect the frustum and can be safely culled.
  ///
  /// This is a conservative test: some geometry that is technically outside
  /// the frustum may still pass (e.g. when no single plane separates all
  /// points), but false positives only cause extra work, never missed draws.
  static bool allPointsOutsideAnyPlane(
      const std::array<glm::vec4, COUNT> &planes,
      const glm::vec3 *points, std::size_t count);

  /// Returns true if any of the supplied points lies inside the frustum
  /// (on the positive side of all six planes). Complement of the classic
  /// point-in-frustum check for multiple samples.
  static bool
  anyPointInside(const std::array<glm::vec4, COUNT> &planes,
                 const glm::vec3 *points, std::size_t count);

  /// Single-point containment: `true` iff `point` lies inside all planes.
  static bool containsPoint(const std::array<glm::vec4, COUNT> &planes,
                            const glm::vec3 &point);

  /// Conservative sphere-vs-frustum rejection. Returns `true` iff the sphere
  /// lies entirely on the outside (negative) side of at least one plane —
  /// i.e. the sphere cannot intersect the frustum and is safe to cull.
  ///
  /// Relies on the normalised-plane convention from `extractPlanes`: because
  /// `plane.xyz` is a unit vector, `dot(plane.xyz, center) + plane.w` is the
  /// signed distance from `center` to the plane (positive on the inside).
  /// The sphere is fully outside a given plane when that distance is less
  /// than `-radius`.
  ///
  /// As with `allPointsOutsideAnyPlane`, this is a one-sided test: false
  /// negatives are possible (a sphere that straddles a corner of the frustum
  /// with no single plane rejecting it passes through), but false positives
  /// are not. Missing a cull costs a draw call; a false positive would be a
  /// rendering bug.
  static bool sphereOutsideAnyPlane(
      const std::array<glm::vec4, COUNT> &planes,
      const glm::vec3 &center, float radius);
};

}  // namespace render
}  // namespace omega
