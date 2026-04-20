// FrustumTest — verifies the Gribb–Hartmann frustum extraction plus the
// point-containment / polygon-rejection helpers. Tests are matrix-free where
// possible so they don't depend on glm's projection conventions beyond the
// standard GL perspective / lookAt pair.

#include <gtest/gtest.h>

#include <render/Camera.h>
#include <render/Frustum.h>
#include <render/PortalCamera.h>
#include <geometry/Portal.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using omega::geometry::Portal;
using omega::render::Camera;
using omega::render::Frustum;
using omega::render::PortalCamera;

namespace {

constexpr float kEps = 1e-3f;

// A simple camera looking down -Z from the origin, typical OpenGL default.
Camera makeForwardCamera(const glm::vec3 &pos = glm::vec3(0.0f, 0.0f, 0.0f)) {
  Camera cam(pos.x, pos.y, pos.z,
             /*upX=*/0.0f, /*upY=*/1.0f, /*upZ=*/0.0f,
             /*yaw=*/-90.0f, /*pitch=*/0.0f);
  cam.setPerspective(60.0f, 800.0f, 600.0f, 0.1f, 100.0f);
  return cam;
}

}  // namespace

TEST(FrustumTest, ExtractedPlaneNormalsAreUnitLength) {
  // The normalisation step should give every plane.xyz unit length. This is
  // a useful invariant for callers that interpret plane.w as signed distance.
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);
  for (const auto &p : planes) {
    const float len = glm::length(glm::vec3(p));
    EXPECT_NEAR(len, 1.0f, kEps)
        << "plane (" << p.x << ", " << p.y << ", " << p.z << ", " << p.w
        << ") not unit length";
  }
}

TEST(FrustumTest, OriginIsInsideOwnFrustum) {
  // A camera at the origin looking down -Z includes points on its -Z axis
  // within the frustum. A point a few units in front of the camera must be
  // inside all six planes.
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);

  EXPECT_TRUE(Frustum::containsPoint(planes, glm::vec3(0.0f, 0.0f, -5.0f)));
}

TEST(FrustumTest, PointBehindCameraIsRejected) {
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);

  // A point behind the camera (positive Z, we look down -Z) must fail the
  // near plane test.
  EXPECT_FALSE(Frustum::containsPoint(planes, glm::vec3(0.0f, 0.0f, 5.0f)));
}

TEST(FrustumTest, PointPastFarPlaneIsRejected) {
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);

  // Far plane is 100. A point at -500 along Z must be rejected.
  EXPECT_FALSE(Frustum::containsPoint(planes, glm::vec3(0.0f, 0.0f, -500.0f)));
}

TEST(FrustumTest, AllPointsOutsideAnyPlaneCullsBehindCamera) {
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);

  // All four points sit behind the camera along +Z, so the near plane
  // rejects them unanimously — the helper must return true.
  const glm::vec3 corners[4] = {
      {-1.0f, -1.0f, 10.0f},
      { 1.0f, -1.0f, 10.0f},
      { 1.0f,  1.0f, 10.0f},
      {-1.0f,  1.0f, 10.0f},
  };
  EXPECT_TRUE(Frustum::allPointsOutsideAnyPlane(planes, corners, 4));
}

TEST(FrustumTest, AllPointsOutsideAnyPlaneKeepsStraddlingPolygon) {
  Camera cam = makeForwardCamera();
  const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
  const auto planes = Frustum::extractPlanes(vp);

  // A polygon centred on the optical axis 5 units in front of the camera —
  // well inside the frustum on every plane. No plane rejects all four
  // points, so the helper should say "not fully outside".
  const glm::vec3 corners[4] = {
      {-0.5f, -0.5f, -5.0f},
      { 0.5f, -0.5f, -5.0f},
      { 0.5f,  0.5f, -5.0f},
      {-0.5f,  0.5f, -5.0f},
  };
  EXPECT_FALSE(Frustum::allPointsOutsideAnyPlane(planes, corners, 4));
}

TEST(FrustumTest, PortalInFrontOfCameraIsVisible) {
  // Integration check: the real PortalCamera::isInViewFrustum should accept
  // a portal placed directly in front of the camera.
  Camera cam = makeForwardCamera();
  Portal portal({0.0f, 0.0f, -5.0f}, {0.0f, 0.0f, 1.0f});
  EXPECT_TRUE(PortalCamera::isInViewFrustum(portal, cam));
}

TEST(FrustumTest, PortalBehindCameraIsCulled) {
  Camera cam = makeForwardCamera();
  // Portal behind the camera (+Z side). The near plane should reject all
  // four corners.
  Portal portal({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, -1.0f});
  EXPECT_FALSE(PortalCamera::isInViewFrustum(portal, cam));
}

TEST(FrustumTest, PortalFarPastFarPlaneIsCulled) {
  Camera cam = makeForwardCamera();
  // Portal placed way beyond the far plane — uniformly rejected by far plane.
  Portal portal({0.0f, 0.0f, -500.0f}, {0.0f, 0.0f, 1.0f});
  EXPECT_FALSE(PortalCamera::isInViewFrustum(portal, cam));
}

TEST(FrustumTest, PortalOffToTheSideIsCulled) {
  Camera cam = makeForwardCamera();
  // Portal placed far to the right of the camera's forward cone.
  // With a 60° FOV, a portal at (+200, 0, -5) is well outside the right
  // plane.
  Portal portal({200.0f, 0.0f, -5.0f}, {-1.0f, 0.0f, 0.0f}, 2.0f, 2.0f);
  EXPECT_FALSE(PortalCamera::isInViewFrustum(portal, cam));
}
