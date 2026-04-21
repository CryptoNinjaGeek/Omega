// PortalRendererStencilTest — verifies the non-GL surface of the Phase 1.4
// option-B stencil renderer: the enable/disable toggle, default state, and
// that toggling it back off is observable. The actual stencil algorithm needs
// a real OpenGL context to exercise and is covered by the Portal demo (run
// with `--stencil-portals` once that flag is wired in the demo).

#include <gtest/gtest.h>

#include <geometry/PortalRenderer.h>

using omega::geometry::PortalRenderer;

TEST(PortalRendererStencilTest, DefaultsToFboMode) {
  PortalRenderer renderer;
  // Stencil mode must default to OFF so existing demos continue to use the
  // FBO pipeline without any opt-in ceremony.
  EXPECT_FALSE(renderer.isStencilMode());
  EXPECT_TRUE(renderer.isEnabled())
      << "PortalRenderer should default to enabled; stencil mode is a sub-toggle";
}

TEST(PortalRendererStencilTest, SetStencilModeRoundTrip) {
  PortalRenderer renderer;
  renderer.setStencilMode(true);
  EXPECT_TRUE(renderer.isStencilMode());
  renderer.setStencilMode(false);
  EXPECT_FALSE(renderer.isStencilMode());
}

TEST(PortalRendererStencilTest, StencilModeIsIndependentOfEnabled) {
  // isEnabled() and isStencilMode() are orthogonal: disabling the renderer
  // should not reset the stencil preference, and toggling stencil should not
  // touch the enabled flag.
  PortalRenderer renderer;
  renderer.setStencilMode(true);
  renderer.setEnabled(false);
  EXPECT_FALSE(renderer.isEnabled());
  EXPECT_TRUE(renderer.isStencilMode());

  renderer.setEnabled(true);
  EXPECT_TRUE(renderer.isEnabled());
  EXPECT_TRUE(renderer.isStencilMode());
}
