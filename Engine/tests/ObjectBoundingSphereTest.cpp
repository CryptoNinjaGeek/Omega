// ObjectBoundingSphereTest — covers the world-space sphere transform used by
// Scene::render for per-object frustum culling (Phase 1.3). These tests are
// intentionally pure-math: they construct a bare Object, set a local sphere,
// mutate `model_` via the public API (`position`, `scale`) and check the
// transformed sphere. No GL context is required.

#include <gtest/gtest.h>

#include <geometry/Object.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using omega::geometry::BoundingSphere;
using omega::geometry::Object;

namespace {

constexpr float kEps = 1e-4f;

bool nearVec3(const glm::vec3 &a, const glm::vec3 &b, float eps = kEps) {
  return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps &&
         std::abs(a.z - b.z) < eps;
}

}  // namespace

TEST(ObjectBoundingSphereTest, UnsetSphereProducesNullopt) {
  Object obj;
  EXPECT_FALSE(obj.boundingSphere().has_value());
  EXPECT_FALSE(obj.worldBoundingSphere().has_value());
}

TEST(ObjectBoundingSphereTest, IdentityModelLeavesSphereUntouched) {
  Object obj;
  obj.setBoundingSphere(glm::vec3(1.0f, 2.0f, 3.0f), 4.0f);

  const auto world = obj.worldBoundingSphere();
  ASSERT_TRUE(world.has_value());
  EXPECT_TRUE(nearVec3(world->center, glm::vec3(1.0f, 2.0f, 3.0f)));
  EXPECT_NEAR(world->radius, 4.0f, kEps);
}

TEST(ObjectBoundingSphereTest, TranslationShiftsCenter) {
  Object obj;
  obj.setBoundingSphere(glm::vec3(0.0f), 1.0f);

  // Object::position(vec3) uses glm::translate on the current model matrix.
  obj.position(glm::vec3(10.0f, -2.0f, 5.0f));

  const auto world = obj.worldBoundingSphere();
  ASSERT_TRUE(world.has_value());
  EXPECT_TRUE(nearVec3(world->center, glm::vec3(10.0f, -2.0f, 5.0f)));
  EXPECT_NEAR(world->radius, 1.0f, kEps);
}

TEST(ObjectBoundingSphereTest, UniformScaleScalesRadiusAndCenter) {
  Object obj;
  obj.setBoundingSphere(glm::vec3(1.0f, 0.0f, 0.0f), 2.0f);

  // Uniform 3x scale applied to identity: model = diag(3,3,3,1).
  obj.scale(3.0f);

  const auto world = obj.worldBoundingSphere();
  ASSERT_TRUE(world.has_value());
  // Local center (1,0,0) → world (3,0,0) under 3x scale.
  EXPECT_TRUE(nearVec3(world->center, glm::vec3(3.0f, 0.0f, 0.0f)));
  EXPECT_NEAR(world->radius, 6.0f, kEps);
}

TEST(ObjectBoundingSphereTest, NonUniformScaleUsesMaxAxisForRadius) {
  // Non-uniform scale is handled conservatively: radius is multiplied by the
  // largest scale factor so the sphere still encloses the transformed
  // geometry (at the cost of being looser than an ellipsoid fit).
  Object obj;
  obj.setBoundingSphere(glm::vec3(0.0f), 1.0f);

  // Build a non-uniform scale directly in the model matrix: x=2, y=5, z=3.
  // We can't hit this path through Object::scale(float) alone — the test
  // exercises worldBoundingSphere's policy explicitly.
  glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 5.0f, 3.0f));
  obj.setModel(model);

  const auto world = obj.worldBoundingSphere();
  ASSERT_TRUE(world.has_value());
  EXPECT_TRUE(nearVec3(world->center, glm::vec3(0.0f)));
  EXPECT_NEAR(world->radius, 5.0f, kEps);  // max of (2, 5, 3)
}

TEST(ObjectBoundingSphereTest, TranslationAfterScaleTransformsCenterCorrectly) {
  // Ensure we use the full 4x4 (not just the rotation/scale block) when
  // mapping the local center.
  Object obj;
  obj.setBoundingSphere(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f);

  // model = T(10, 0, 0) * S(2): local (1,0,0) → world (12, 0, 0).
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
  model = glm::scale(model, glm::vec3(2.0f));
  obj.setModel(model);

  const auto world = obj.worldBoundingSphere();
  ASSERT_TRUE(world.has_value());
  EXPECT_TRUE(nearVec3(world->center, glm::vec3(12.0f, 0.0f, 0.0f)));
  EXPECT_NEAR(world->radius, 2.0f, kEps);
}

TEST(ObjectBoundingSphereTest, ClearBoundingSphereResetsBack) {
  Object obj;
  obj.setBoundingSphere(glm::vec3(0.0f), 5.0f);
  EXPECT_TRUE(obj.boundingSphere().has_value());

  obj.clearBoundingSphere();
  EXPECT_FALSE(obj.boundingSphere().has_value());
  EXPECT_FALSE(obj.worldBoundingSphere().has_value());
}
