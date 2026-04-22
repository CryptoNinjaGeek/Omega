// PropCollidersTest — covers the analytical sphere-vs-cylinder push-out used
// by the Outdoor demo to stop the player camera and wandering NPCs from
// walking through placed vegetation/rocks. Pure math: no ObjectTree involved
// here (the scene-side `addFromPlacedNode` path is exercised by the demo).

#include <gtest/gtest.h>

#include <cmath>

#include <world/PropColliders.h>

#include <glm/glm.hpp>

using omega::world::PropColliderSet;

namespace {

constexpr float kEps = 1e-4f;

}  // namespace

TEST(PropCollidersTest, EmptySetReturnsInputUnchanged) {
  PropColliderSet set;
  const glm::vec2 in(3.0f, 4.0f);
  const glm::vec2 out = set.resolveXZ(in, 0.5f);
  EXPECT_NEAR(out.x, in.x, kEps);
  EXPECT_NEAR(out.y, in.y, kEps);
  EXPECT_EQ(set.size(), 0u);
}

TEST(PropCollidersTest, DegenerateRadiusDropped) {
  PropColliderSet set;
  set.add(glm::vec3(0.0f), 0.0f);    // zero radius — skipped
  set.add(glm::vec3(0.0f), -1.0f);   // negative — skipped
  set.add(glm::vec3(0.0f), 1.0f);    // kept
  EXPECT_EQ(set.size(), 1u);
}

TEST(PropCollidersTest, ActorOutsideSphereUnchanged) {
  PropColliderSet set;
  set.add(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
  // Actor at (5, 5) is far outside the sphere at origin with radius 1.
  // With actorRadius 0.5 the keep-out distance is 1.5, still far away.
  const glm::vec2 out = set.resolveXZ(glm::vec2(5.0f, 5.0f), 0.5f);
  EXPECT_NEAR(out.x, 5.0f, kEps);
  EXPECT_NEAR(out.y, 5.0f, kEps);
}

TEST(PropCollidersTest, ActorInsideSpherePushedToBoundary) {
  PropColliderSet set;
  set.add(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
  // Actor at (0.4, 0) with actorRadius 0.2 → needs distance 1.2 from centre.
  const glm::vec2 out = set.resolveXZ(glm::vec2(0.4f, 0.0f), 0.2f);
  EXPECT_NEAR(out.x, 1.2f, kEps);
  EXPECT_NEAR(out.y, 0.0f, kEps);
}

TEST(PropCollidersTest, PushRespectsSeparationDirection) {
  PropColliderSet set;
  set.add(glm::vec3(10.0f, 0.0f, 10.0f), 2.0f);
  // Actor 1 unit away from sphere centre toward +X +Z (same diagonal).
  // Separation direction is normalized(actor - centre) = (√2/2, √2/2).
  // Min dist is r + actorR = 2.0 + 0.25 = 2.25.
  const glm::vec2 out =
      set.resolveXZ(glm::vec2(10.0f + std::sqrt(0.5f),
                              10.0f + std::sqrt(0.5f)),
                    0.25f);
  // Result sits at centre + 2.25 * normalized direction.
  const float expected = 10.0f + 2.25f * std::sqrt(0.5f);
  EXPECT_NEAR(out.x, expected, kEps);
  EXPECT_NEAR(out.y, expected, kEps);
}

TEST(PropCollidersTest, TwoOverlappingSpheresResolveWithIterations) {
  // Two spheres tangent on the X axis; the actor sits in the narrow crossing
  // zone between them. A non-zero actor radius makes the keep-out circles
  // genuinely overlap (the geometric spheres themselves are only tangent), and
  // a small Z offset breaks the perfect radial symmetry so the iterative push
  // can converge perpendicular to the contact axis instead of oscillating
  // along it. A single iteration against one sphere will re-overlap the
  // other; multiple iterations should land the actor outside both keep-out
  // circles — i.e. at least one sphere-boundary distance away from the
  // contact axis at the origin.
  PropColliderSet set;
  set.add(glm::vec3(-1.0f, 0.0f, 0.0f), 1.0f);
  set.add(glm::vec3( 1.0f, 0.0f, 0.0f), 1.0f);
  // Actor at (0, 0.1) with radius 0.5 — keep-out distance from each centre is
  // 1.5, well inside the actor's starting position of distance ~1.005 from
  // each sphere centre.
  const glm::vec2 out = set.resolveXZ(glm::vec2(0.0f, 0.1f), 0.5f, 4);
  // We don't mandate a specific direction for symmetric pushes, but the
  // magnitude must be at least 1.0 (the sphere boundary) in some direction.
  EXPECT_GE(glm::length(out), 1.0f - kEps);
  // And the actor must end up outside both keep-out circles.
  EXPECT_GE(glm::length(out - glm::vec2(-1.0f, 0.0f)), 1.5f - kEps);
  EXPECT_GE(glm::length(out - glm::vec2( 1.0f, 0.0f)), 1.5f - kEps);
}

TEST(PropCollidersTest, ActorExactlyOnSphereCentreGetsNonZeroPush) {
  PropColliderSet set;
  set.add(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
  // Exact-centre special case must not NaN and must produce at least
  // (r + actorR) of separation.
  const glm::vec2 out = set.resolveXZ(glm::vec2(0.0f, 0.0f), 0.25f);
  EXPECT_TRUE(std::isfinite(out.x));
  EXPECT_TRUE(std::isfinite(out.y));
  EXPECT_GE(glm::length(out), 1.25f - kEps);
}

TEST(PropCollidersTest, YCoordIgnoredForXZPush) {
  // A sphere centred well above the actor is still a collider in XZ. The
  // intent is "you can't walk through a tree even if its bounding sphere
  // happens to be centred high in the canopy" — exactly the case that
  // matters for the Outdoor demo.
  PropColliderSet set;
  set.add(glm::vec3(0.0f, 10.0f, 0.0f), 1.0f);
  const glm::vec2 out = set.resolveXZ(glm::vec2(0.2f, 0.0f), 0.2f);
  EXPECT_NEAR(out.x, 1.2f, kEps);
  EXPECT_NEAR(out.y, 0.0f, kEps);
}

TEST(PropCollidersTest, NegativeActorRadiusIsNoOp) {
  // Defensive: a bogus negative actorRadius must not reflect the actor or
  // do anything else surprising — the simplest safe behaviour is to return
  // the input untouched.
  PropColliderSet set;
  set.add(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
  const glm::vec2 out = set.resolveXZ(glm::vec2(0.1f, 0.0f), -0.5f);
  EXPECT_NEAR(out.x, 0.1f, kEps);
  EXPECT_NEAR(out.y, 0.0f, kEps);
}
