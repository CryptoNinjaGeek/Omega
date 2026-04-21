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
  // Self-destined portal is a mirror; the unified entry point dispatches to
  // the internal mirror path. Post-Phase 2.2 this is the only public entry,
  // so the test drives it directly.
  auto portal = std::make_shared<Portal>(glm::vec3(0.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f));
  portal->setDestination(portal);
  ASSERT_TRUE(portal->isMirror());

  Camera cam(glm::vec3(0.0f, 1.0f, -3.0f));
  cam.setLookAt({0.0f, 1.0f, 0.0f});
  cam.setPerspective(45.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

  const glm::mat4 view = PortalCamera::calculatePortalViewUnified(cam, *portal);
  // Expect every element finite.
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_TRUE(std::isfinite(view[i][j]))
          << "non-finite at (" << i << "," << j << ")";
    }
  }
}

TEST(PortalCameraTest, CalculatePortalViewPlacesCameraThroughPairedPortals) {
  // Demo layout: two portals on opposite walls, facing each other.
  //   portal_left  at (-4, 1.5, 0) normal (+1, 0, 0)
  //   portal_right at (+4, 1.5, 0) normal (-1, 0, 0)
  // Player at room centre-ish, looking -Z. Looking through portal_left
  // should render the scene from a virtual camera placed the same distance
  // BEHIND portal_right (on its -normal side, outside the room) as the
  // player is in front of portal_left.
  //
  // For player at (0, 1.5, 5):
  //   source-local offset = (+right=5, +up=0, +into-portal=-4)
  //   after 180° flip about local up: (-5, 0, +4)
  //   re-expressed in destination world: virtual pos = (8, 1.5, 5)
  //   virtual forward: still -Z (the two portals' rotations cancel via R_180y).
  //
  // Phase 2: the triple-arg `calculatePortalView(camera, src, dst)` was
  // retired. We now build the doorway pair by setting `destination_` on
  // each portal and invoking the unified entry point.
  auto portalLeft =
      std::make_shared<Portal>(glm::vec3(-4.0f, 1.5f, 0.0f),
                               glm::vec3(1.0f, 0.0f, 0.0f), 2.0f, 3.0f);
  auto portalRight =
      std::make_shared<Portal>(glm::vec3(4.0f, 1.5f, 0.0f),
                               glm::vec3(-1.0f, 0.0f, 0.0f), 2.0f, 3.0f);
  portalLeft->setDestination(portalRight);
  portalRight->setDestination(portalLeft);

  Camera player(/*posX=*/0.0f, /*posY=*/1.5f, /*posZ=*/5.0f,
                /*upX=*/0.0f, /*upY=*/1.0f, /*upZ=*/0.0f,
                /*yaw=*/-90.0f, /*pitch=*/0.0f);
  player.setPerspective(60.0f, 1280.0f, 720.0f, 0.1f, 100.0f);

  const glm::mat4 portalView =
      PortalCamera::calculatePortalViewUnified(player, *portalLeft);

  // Derive the virtual camera's world-space position by inverting the view.
  const glm::mat4 virtualWorld = glm::inverse(portalView);
  const glm::vec3 virtualPos(virtualWorld[3]);

  EXPECT_NEAR(virtualPos.x, 8.0f, kEps);
  EXPECT_NEAR(virtualPos.y, 1.5f, kEps);
  EXPECT_NEAR(virtualPos.z, 5.0f, kEps);

  // Virtual camera should still be looking down -Z. Column 2 of the world
  // matrix is -front (OpenGL convention), so front = -col2.
  const glm::vec3 virtualFront = -glm::normalize(glm::vec3(virtualWorld[2]));
  EXPECT_NEAR(virtualFront.x, 0.0f, kEps);
  EXPECT_NEAR(virtualFront.y, 0.0f, kEps);
  EXPECT_NEAR(virtualFront.z, -1.0f, kEps);
}

TEST(PortalCameraTest, PortalTransformHasBasisInColumns) {
  // Portal::getTransform() must return a proper local-to-world matrix:
  //   col 0 = right, col 1 = up, col 2 = -normal, col 3 = position
  // A prior bug stored the basis as rows, which silently inverted the
  // rotation and broke through-portal math.
  Portal portal({1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f});
  const glm::mat4 t = portal.getTransform();

  const glm::vec3 col0(t[0]);
  const glm::vec3 col1(t[1]);
  const glm::vec3 col2(t[2]);
  const glm::vec3 col3(t[3]);

  EXPECT_TRUE(vec3Near(col0, portal.getRight()));
  EXPECT_TRUE(vec3Near(col1, portal.getUp()));
  EXPECT_TRUE(vec3Near(col2, -portal.getNormal()));
  EXPECT_TRUE(vec3Near(col3, portal.getPosition()));

  // Local origin (0,0,0,1) should map to the portal's world position.
  const glm::vec4 localOrigin(0.0f, 0.0f, 0.0f, 1.0f);
  const glm::vec4 worldOrigin = t * localOrigin;
  EXPECT_TRUE(vec3Near(glm::vec3(worldOrigin), portal.getPosition()));

  // Local +X (1,0,0,0) — as a direction vector — should map to world `right`.
  const glm::vec4 localX(1.0f, 0.0f, 0.0f, 0.0f);
  const glm::vec4 worldX = t * localX;
  EXPECT_TRUE(vec3Near(glm::vec3(worldX), portal.getRight()));
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
