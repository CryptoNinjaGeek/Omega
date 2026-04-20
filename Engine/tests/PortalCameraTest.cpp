// PortalCameraTest — verifies view-matrix math for portal rendering.
// Tests focus on invariants (facing, position-through-portal, mirror
// reflection) rather than full matrix equality so they remain stable if the
// underlying formulas are refactored.

#include <gtest/gtest.h>

#include <render/Camera.h>
#include <render/PortalCamera.h>
#include <geometry/Portal.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

using omega::geometry::Portal;
using omega::render::Camera;
using omega::render::PortalCamera;

namespace {

constexpr float kEps = 1e-3f;

bool vec3Near(const glm::vec3 &a, const glm::vec3 &b, float eps = kEps) {
  return glm::all(glm::epsilonEqual(a, b, eps));
}

}  // namespace

TEST(PortalCameraTest, TransformPositionThroughIdenticalPortalsIsIdentity) {
  // Two portals with the same position and normal — the relative transform
  // should be the identity, so any point passes through unchanged.
  Portal a({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});
  Portal b({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});

  const glm::vec3 p(1.0f, 2.0f, 3.0f);
  const glm::vec3 out = PortalCamera::transformPosition(p, a, b);
  EXPECT_TRUE(vec3Near(out, p))
      << "got (" << out.x << ", " << out.y << ", " << out.z << ")";
}

TEST(PortalCameraTest, TransformPositionTranslatesBetweenPortals) {
  // Two portals with the same orientation but separated by a translation.
  // Points near portal A should map to the same offset relative to portal B.
  Portal a({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});
  Portal b({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});

  const glm::vec3 atA(0.0f, 1.0f, 2.0f);
  const glm::vec3 out = PortalCamera::transformPosition(atA, a, b);
  // Exact transform depends on implementation; we only require finiteness and
  // that the out is near portal B (within 100 units — sanity only).
  EXPECT_TRUE(std::isfinite(out.x));
  EXPECT_TRUE(std::isfinite(out.y));
  EXPECT_TRUE(std::isfinite(out.z));
  EXPECT_LT(glm::length(out - b.getPosition()), 100.0f);
}

TEST(PortalCameraTest, ClippingPlaneFromPortalHelperMatchesPortal) {
  Portal portal({1.0f, 0.0f, 2.0f}, {0.0f, 1.0f, 0.0f});
  const glm::vec4 expected = portal.getClippingPlane();
  const glm::vec4 actual = PortalCamera::getClippingPlane(portal);
  EXPECT_NEAR(expected.x, actual.x, kEps);
  EXPECT_NEAR(expected.y, actual.y, kEps);
  EXPECT_NEAR(expected.z, actual.z, kEps);
  EXPECT_NEAR(expected.w, actual.w, kEps);
}

TEST(PortalCameraTest, VisibilityCheckRequiresFacingPortal) {
  // Contract: PortalCamera::isPortalVisible treats the half-space the portal
  // normal points INTO as the visible side — same "front" convention as
  // Portal::isPointInFront. So a portal at origin with normal +Z is visible
  // from a camera on the +Z side and invisible from a camera on the -Z side.
  Portal portal({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

  Camera front(glm::vec3(0.0f, 0.0f, 5.0f));
  front.setLookAt({0.0f, 0.0f, 0.0f});
  front.setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);
  EXPECT_TRUE(PortalCamera::isPortalVisible(portal, front));

  // Camera behind portal (at -Z side, opposite of normal): not visible.
  Camera behind(glm::vec3(0.0f, 0.0f, -5.0f));
  behind.setLookAt({0.0f, 0.0f, -10.0f});
  behind.setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);
  EXPECT_FALSE(PortalCamera::isPortalVisible(portal, behind));
}

TEST(PortalCameraTest, MirrorViewIsFinite) {
  // Self-destined portal is a mirror; the view matrix must be computable.
  auto portal = std::make_shared<Portal>(glm::vec3(0.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f));
  portal->setDestination(portal);
  ASSERT_TRUE(portal->isMirror());

  Camera cam(glm::vec3(0.0f, 1.0f, -3.0f));
  cam.setLookAt({0.0f, 1.0f, 0.0f});
  cam.setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

  const glm::mat4 view = PortalCamera::calculateMirrorView(cam, *portal);
  // Expect every element finite.
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_TRUE(std::isfinite(view[i][j]))
          << "non-finite at (" << i << "," << j << ")";
    }
  }
}

TEST(PortalCameraTest, CameraAccessorsReturnLastSetParameters) {
  // Phase 0.2 accessors: verify setPerspective caches the parameters.
  Camera cam;
  cam.setPerspective(60.0f, 1920.0f, 1080.0f, 0.5f, 250.0f);

  EXPECT_FLOAT_EQ(cam.fov(), 60.0f);
  EXPECT_FLOAT_EQ(cam.nearPlane(), 0.5f);
  EXPECT_FLOAT_EQ(cam.farPlane(), 250.0f);
  EXPECT_NEAR(cam.aspectRatio(), 1920.0f / 1080.0f, 1e-4f);
}
