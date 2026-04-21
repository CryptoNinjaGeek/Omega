// PortalTransformTest — verifies basic geometric invariants of the Portal
// class. These tests intentionally exercise pure math only (no GL context),
// so they run on CI headlessly without a windowing system.
//
// Part of Phase 0.4 scaffolding: establishes a baseline so later phases
// (clipping plane, frustum culling) can add regression tests in the same
// suite.

#include <gtest/gtest.h>

#include <geometry/Portal.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

using omega::geometry::Portal;

namespace {

constexpr float kEps = 1e-4f;

bool vec3Near(const glm::vec3 &a, const glm::vec3 &b, float eps = kEps) {
  return glm::all(glm::epsilonEqual(a, b, eps));
}

}  // namespace

TEST(PortalTransformTest, DefaultConstructionHasIdentityOrientation) {
  Portal portal;
  EXPECT_TRUE(vec3Near(portal.getPosition(), glm::vec3(0.0f)));
  // Default normal is -Z ("facing forward").
  EXPECT_TRUE(vec3Near(portal.getNormal(), glm::vec3(0.0f, 0.0f, -1.0f)));
  // Default size (2x2) and state (open + passable + visible).
  EXPECT_FLOAT_EQ(portal.getWidth(), 2.0f);
  EXPECT_FLOAT_EQ(portal.getHeight(), 2.0f);
  EXPECT_TRUE(portal.isOpen());
  EXPECT_TRUE(portal.isPassable());
  EXPECT_TRUE(portal.isVisible());
  EXPECT_TRUE(portal.isEnabled());
}

TEST(PortalTransformTest, SettingNormalRebuildsBasis) {
  Portal portal({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
  // Normal pointing +X → up should stay +Y, right should be perpendicular.
  EXPECT_TRUE(vec3Near(portal.getNormal(), glm::vec3(1.0f, 0.0f, 0.0f)));
  // Right and up must each be unit length and perpendicular to the normal.
  EXPECT_NEAR(glm::length(portal.getUp()), 1.0f, kEps);
  EXPECT_NEAR(glm::length(portal.getRight()), 1.0f, kEps);
  EXPECT_NEAR(glm::dot(portal.getNormal(), portal.getUp()), 0.0f, kEps);
  EXPECT_NEAR(glm::dot(portal.getNormal(), portal.getRight()), 0.0f, kEps);
  EXPECT_NEAR(glm::dot(portal.getUp(), portal.getRight()), 0.0f, kEps);
}

TEST(PortalTransformTest, FrontBackClassificationMatchesNormal) {
  // Portal at origin facing +Z, so +Z half-space is "in front".
  Portal portal({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
  EXPECT_TRUE(portal.isPointInFront({0.0f, 0.0f, 5.0f}));
  EXPECT_FALSE(portal.isPointInFront({0.0f, 0.0f, -5.0f}));
}

TEST(PortalTransformTest, DistanceToPlaneIsSigned) {
  Portal portal({0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});  // floor-aligned
  EXPECT_NEAR(portal.distanceToPlane({0.0f, 4.0f, 0.0f}), 3.0f, kEps);
  // Point below the plane should be negative distance.
  EXPECT_LT(portal.distanceToPlane({0.0f, -2.0f, 0.0f}), 0.0f);
  EXPECT_NEAR(portal.distanceToPlane({0.0f, 1.0f, 0.0f}), 0.0f, kEps);
}

TEST(PortalTransformTest, ClippingPlaneEquationMatchesNormalAndPosition) {
  // Contract of Portal::getClippingPlane():
  //   - plane.xyz is the *negated* portal normal (points away from the
  //     portal's front, so the plane clips geometry behind the portal).
  //   - plane.w = dot(plane.xyz, position), so the plane equation is
  //     dot(plane.xyz, X) - plane.w = 0 (not the canonical `+ w = 0` form).
  Portal portal({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f});
  const glm::vec4 plane = portal.getClippingPlane();

  // The xyz component is the *opposite* of the portal's normal.
  EXPECT_TRUE(vec3Near(glm::vec3(plane), -portal.getNormal()));

  // The portal's position lies on the plane: dot(plane.xyz, p) == plane.w.
  const glm::vec3 p = portal.getPosition();
  const float onPlane = plane.x * p.x + plane.y * p.y + plane.z * p.z - plane.w;
  EXPECT_NEAR(onPlane, 0.0f, kEps);
}

TEST(PortalTransformTest, MirrorFlagSetWhenDestinationIsSelf) {
  auto portal = std::make_shared<Portal>();
  EXPECT_FALSE(portal->isMirror());
  portal->setDestination(portal);
  EXPECT_TRUE(portal->isMirror());
}

TEST(PortalTransformTest, MirrorOverlayDefaultsToOff) {
  Portal portal;
  EXPECT_FALSE(portal.hasMirrorOverlay());
  // Default tint is white so that enabling the overlay with no custom
  // tint is a no-op visually. Intensity default is a non-zero 0.5f so
  // "flip the flag" gives a visible mix without additional setup.
  EXPECT_TRUE(vec3Near(portal.getMirrorTint(), glm::vec3(1.0f)));
  EXPECT_FLOAT_EQ(portal.getMirrorIntensity(), 0.5f);
}

TEST(PortalTransformTest, SetMirrorOverlayStoresTintAndIntensity) {
  Portal portal;
  // Phase 1.5 contract: setMirrorOverlay(enabled, intensity, tint) stores
  // all three values so portal.fs can sample them per-frame.
  portal.setMirrorOverlay(true, 0.8f, glm::vec3(0.6f, 0.7f, 0.8f));
  EXPECT_TRUE(portal.hasMirrorOverlay());
  EXPECT_FLOAT_EQ(portal.getMirrorIntensity(), 0.8f);
  EXPECT_TRUE(vec3Near(portal.getMirrorTint(), glm::vec3(0.6f, 0.7f, 0.8f)));

  // Flipping off retains the previously-set tint — downstream code may
  // toggle the flag independently without re-authoring the tint.
  portal.setMirrorOverlay(false, 0.8f, glm::vec3(0.6f, 0.7f, 0.8f));
  EXPECT_FALSE(portal.hasMirrorOverlay());
  EXPECT_TRUE(vec3Near(portal.getMirrorTint(), glm::vec3(0.6f, 0.7f, 0.8f)));
}

TEST(PortalTransformTest, FourCornersAreCoplanarWithPortalCenter) {
  // width=4, height=2, centered at (5, 1, -2) facing -Z. Off-origin so the
  // plane-equation check actually constrains w (at the origin, w = 0 and the
  // sign convention is invisible).
  Portal portal({5.0f, 1.0f, -2.0f}, {0.0f, 0.0f, -1.0f}, 4.0f, 2.0f);
  glm::vec3 corners[4];
  portal.getCorners(corners);
  // All corners must lie in the portal's plane. Plane contract:
  // dot(plane.xyz, X) - plane.w == 0 (see ClippingPlaneEquation test).
  const glm::vec4 plane = portal.getClippingPlane();
  for (int i = 0; i < 4; ++i) {
    const float dist =
        plane.x * corners[i].x + plane.y * corners[i].y +
        plane.z * corners[i].z - plane.w;
    EXPECT_NEAR(dist, 0.0f, kEps) << "corner " << i << " off-plane";
  }
}
