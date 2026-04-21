#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <render/Texture.h>
#include <render/Shader.h>
#include <render/PointLight.h>
#include <render/DirectionalLight.h>
#include <render/SpotLight.h>

#include <geometry/ObjectTree.h>
#include <geometry/Object.h>
#include <geometry/Vertex.h>
#include <utils/ObjectGenerator.h>

#include <reactphysics3d/reactphysics3d.h>

#include <render/Frustum.h>

#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace omega::render;
using namespace omega::interface;

namespace omega {
namespace geometry {

// Forward declaration to break circular dependency
class PortalRenderer;

class Scene {
public:
  // constructor, expects a filepath to a 3D model.
  explicit Scene(bool gamma = false);
  explicit Scene(std::string const &path, bool gamma = false);

  auto import(std::string const &path) -> void;

  void render();
  void render(std::shared_ptr<render::Camera> camera);
  void shaders(std::shared_ptr<render::Shader> shader,
			   std::shared_ptr<render::Shader> lightShader);
  void lights(std::vector<std::shared_ptr<Light>>);
  auto object(std::string) -> std::shared_ptr<Object>;
  auto scale(float) -> void;
  auto process(float) -> void;
  auto prepare() -> void;
  auto debug(bool val) -> void;

  auto add(std::shared_ptr<ObjectNode> tree) ->void;
  auto add(std::shared_ptr<Object> object) ->void;
  auto add(std::shared_ptr<Light> light) -> void { lights_.push_back(light); }
  auto add(std::shared_ptr<Camera> camera) -> unsigned int {
	cameras_.push_back(camera);
	return cameras_.size() - 1;
  }

  auto setCurrentCamera(unsigned int index) -> void;
  auto currentCamera() -> std::shared_ptr<Camera> { return cameras_[current_camera_]; }

  // Per-frame render statistics produced during the most recent main-scene
  // traversal (Scene::render(camera)). Intended as a cheap perf signal for
  // demos/debug overlays — not a stable public API.
  //
  //   considered      : every Object visited during the traversal.
  //   drawn           : objects whose `render(camera)` was actually issued.
  //   culledFrustum   : rejected because their bounding sphere lay fully
  //                     outside at least one frustum plane.
  //   drawnNoBounds   : drawn without a cull test because the Object had no
  //                     bounding sphere (SkyBox, hand-authored sub-meshes).
  //                     Tracked separately so you can tell "would have been
  //                     culled but couldn't" from "genuinely visible".
  //   clippingActive  : true iff GL_CLIP_DISTANCE0 was on for the pass — i.e.
  //                     the vertex shader is clipping behind a portal plane.
  struct RenderStats {
    int considered{0};
    int drawn{0};
    int culledFrustum{0};
    int drawnNoBounds{0};
    bool clippingActive{false};
  };
  const RenderStats& lastRenderStats() const { return lastRenderStats_; }

  // Performance logging toggle. When enabled, the scene accumulates the dt
  // values handed to `process()` and the frame count completed by `render()`,
  // and emits a single `perf` log line once per ~1 s summarising FPS + the
  // most recent RenderStats. Off by default; turn on for demos, perf runs,
  // and debugging. Cadence is fixed at 1 s — the goal is a steady human-
  // readable stream, not a profiler.
  //
  // The report appears via the standard Log facility under the "scene"
  // subsystem (`OMEGA_LOG_INFO("scene", ...)`), matching the rest of the
  // engine's diagnostic output so it can be filtered the same way.
  void setPerformanceLogging(bool enabled) { perfLoggingEnabled_ = enabled; }
  bool isPerformanceLoggingEnabled() const { return perfLoggingEnabled_; }

  // Portal rendering support
  void setPortalRenderer(std::shared_ptr<PortalRenderer> renderer) { portalRenderer_ = renderer; }
  std::shared_ptr<PortalRenderer> getPortalRenderer() const { return portalRenderer_; }

  // Clipping-plane state pushed into the mesh shader during Scene::render().
  //
  // Set by PortalRenderer before rendering a portal view into its FBO so
  // geometry behind the destination portal is culled. The plane follows the
  // engine convention (see Portal::getClippingPlane): plane equation is
  // `dot(plane.xyz, X) - plane.w = 0`, fragments with gl_ClipDistance[0] >= 0
  // are kept. When `enabled` is false the clipping plane is inert
  // (gl_ClipDistance[0] is forced to 1.0 in the vertex shader).
  void setActiveClippingPlane(const glm::vec4& plane, bool enabled) {
    clippingPlane_ = plane;
    clippingEnabled_ = enabled;
  }
  glm::vec4 activeClippingPlane() const { return clippingPlane_; }
  bool isClippingEnabled() const { return clippingEnabled_; }
private:
  void loadModel(std::string const &path);
  auto prepare(ObjectNodePtr node) -> void;
  auto render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera) -> void;
  auto process(ObjectNodePtr node) -> void;
  auto object(std::string name, ObjectNodePtr node) -> std::shared_ptr<Object>;
  void shaders(std::shared_ptr<render::Shader> shader, ObjectNodePtr node);
  void lights(std::vector<std::shared_ptr<Light>> lights, ObjectNodePtr node);

protected:
  ObjectNodePtr _root;

  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Camera>> cameras_;
  std::shared_ptr<Shader> lightShader_;
  std::shared_ptr<Shader> meshShader_;

  unsigned int current_camera_{0};

  // logging and memory management
  reactphysics3d::PhysicsCommon physics_common_;

  // Create a physics world
  reactphysics3d::PhysicsWorld *physics_world_;

  bool gammaCorrection{false};
  bool debug_{false};
  
  // Portal rendering
  std::shared_ptr<PortalRenderer> portalRenderer_{nullptr};

  // Clipping plane state (see setActiveClippingPlane).
  glm::vec4 clippingPlane_{0.0f, 1.0f, 0.0f, 0.0f};
  bool clippingEnabled_{false};

  // Per-pass view-frustum cache. Populated at the top of each
  // `Scene::render(camera)` call and consulted by the ObjectNode traversal
  // to skip any object whose bounding sphere is fully outside the frustum.
  // Reset between passes so nested calls (main scene vs. portal views) do
  // not leak planes from one into the other.
  std::array<glm::vec4, render::Frustum::COUNT> activeFrustumPlanes_{};
  bool activeFrustumValid_{false};

  // Stats accumulated during the current pass, published to `lastRenderStats_`
  // when `render(camera)` returns. Kept separate so readers don't see
  // half-filled numbers if they race with rendering (not that we're threaded,
  // but the invariant is cheap to preserve).
  RenderStats currentRenderStats_{};
  RenderStats lastRenderStats_{};

  // Performance logging state. `perfAccumSeconds_` is fed by `process(dt)` and
  // `perfFrameCount_` is incremented by `render()`. When the accumulator crosses
  // 1 second we emit a log line and reset both counters. See
  // setPerformanceLogging().
  bool perfLoggingEnabled_{false};
  float perfAccumSeconds_{0.0f};
  int perfFrameCount_{0};
};
}  // namespace geometry
}  // namespace omega
