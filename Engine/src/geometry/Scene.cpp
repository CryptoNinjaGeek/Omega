#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <geometry/Scene.h>
#include <geometry/PortalRenderer.h>
#include <render/Frustum.h>
#include <system/FileSystem.h>
#include <system/Log.h>
#include <system/TextureManager.h>
#include <utils/Loader.h>
#include <render/Camera.h>

using namespace std;
using namespace omega::render;
using namespace omega::system;
using namespace omega::interface;
using namespace omega::utils;

Scene::Scene(bool gamma) : gammaCorrection(gamma) {
  physics_world_ = physics_common_.createPhysicsWorld();
  physics_world_->setGravity(reactphysics3d::Vector3(0, -9.81f, 0));
}

Scene::Scene(std::string const &path, bool gamma) : gammaCorrection(gamma) {
  physics_world_ = physics_common_.createPhysicsWorld();

  auto gravity = physics_world_->getGravity();
  OMEGA_LOG_DEBUG("scene", "Gravity: {} {} {}", gravity.x, gravity.y,
				  gravity.z);

  loadModel(path);
}

auto Scene::import(std::string const &path) -> void {
loadModel(path);
if(meshShader_ && lightShader_)
shaders(meshShader_, lightShader_);
}

auto Scene::debug(bool val) -> void {
  debug_ = val;

  physics_world_->setIsDebugRenderingEnabled(debug_);
}

auto Scene::add(std::shared_ptr<Object> object) -> void
{
  if(_root ==nullptr)
	_root = std::make_shared<ObjectNode>();

  _root->meshes.push_back(object);
}

void Scene::loadModel(string const &path) {
  auto tree = Loader::loadModel(path);

  add(tree);
}

auto Scene::add(std::shared_ptr<ObjectNode> tree) ->void
{
  if(_root ==nullptr)
	_root = std::make_shared<ObjectNode>();

  _root->children.push_back(tree);
}

auto Scene::prepare() -> void {
  prepare(_root);
}

auto Scene::prepare(ObjectNodePtr node) -> void {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	object->affectedByLights(lights_);
	object->setupPhysics(physics_world_, &physics_common_);
  }

  for (auto child : node->children)
	prepare(child);
}

void Scene::render() {
  auto camera = cameras_[current_camera_];

  // Phase 1.4 option B: when stencil mode is enabled on the renderer the
  // entire portal pass collapses into a single recursive draw against the
  // main framebuffer. The stencil path is responsible for clearing color +
  // depth + stencil itself, drawing every portal "window" via stencil
  // masking, and rendering the world at each recursion level — so we skip
  // both the FBO pre-pass and the surface post-pass below.
  if (portalRenderer_ && portalRenderer_->isEnabled() &&
      portalRenderer_->isStencilMode()) {
    std::shared_ptr<Scene> scenePtr(this, [](Scene *) {});  // non-owning
    portalRenderer_->renderWithStencil(scenePtr, camera);
    return;
  }

  // Render portal views first (to framebuffers) - BEFORE main scene
  // Note: We pass 'this' as shared_ptr - caller must ensure Scene is managed by shared_ptr
  if (portalRenderer_ && portalRenderer_->isEnabled()) {
    // Create temporary shared_ptr for portal rendering
    // In practice, Scene should be managed by shared_ptr from the start
    std::shared_ptr<Scene> scenePtr(this, [](Scene*){});  // Non-owning shared_ptr
    portalRenderer_->renderPortals(scenePtr, camera, meshShader_);
  }

  // Render main scene
  this->render(camera);

  // Render portal surfaces AFTER main scene so they appear on top
  // Pass nullptr to let PortalRenderer create/use the portal shader
  if (portalRenderer_ && portalRenderer_->isEnabled()) {
    portalRenderer_->renderPortalSurfaces(camera, nullptr);
  }

  // Per-frame performance report. Counts this call as one frame and, once the
  // accumulated dt fed through `process()` crosses the 1-second mark, emits a
  // single line summarising FPS + the last main-pass RenderStats. Kept inside
  // the toggle so production runs pay nothing when it's off.
  if (perfLoggingEnabled_) {
    ++perfFrameCount_;
    if (perfAccumSeconds_ >= 1.0f) {
      const float fps = (perfAccumSeconds_ > 0.0f)
                            ? float(perfFrameCount_) / perfAccumSeconds_
                            : 0.0f;
      const float msPerFrame =
          (perfFrameCount_ > 0)
              ? (perfAccumSeconds_ * 1000.0f) / float(perfFrameCount_)
              : 0.0f;
      const auto &s = lastRenderStats_;
      const int culledPct =
          (s.considered > 0)
              ? int((100.0f * float(s.culledFrustum)) / float(s.considered))
              : 0;
      OMEGA_LOG_INFO(
          "scene",
          "perf FPS={:.1f} ({:.2f} ms) | draw={} cull={} ({}%) "
          "nobounds={} considered={} clip={}",
          fps, msPerFrame, s.drawn, s.culledFrustum, culledPct,
          s.drawnNoBounds, s.considered, s.clippingActive ? "on" : "off");
      perfAccumSeconds_ = 0.0f;
      perfFrameCount_ = 0;
    }
  }
}

// draws the model, and thus all its meshes
void Scene::render(std::shared_ptr<render::Camera> camera) {
  // The clipping plane is a per-frame Scene property pushed by
  // PortalRenderer before rendering a portal view. We toggle
  // GL_CLIP_DISTANCE0 once for the whole traversal rather than per-object
  // so nested scene::render calls (main scene vs portal view) remain
  // independent and cheap.
  if (clippingEnabled_) {
    glEnable(GL_CLIP_DISTANCE0);
  } else {
    glDisable(GL_CLIP_DISTANCE0);
  }

  // Reset per-pass render stats. These are accumulated during the
  // ObjectNode traversal below and published as `lastRenderStats_` once
  // the pass completes so external readers see a consistent snapshot.
  currentRenderStats_ = RenderStats{};
  currentRenderStats_.clippingActive = clippingEnabled_;

  // Extract the six world-space frustum planes once per pass and reuse them
  // for every object. Doing this at the pass boundary (rather than inside
  // the per-object loop) means nested passes — main scene vs. each portal
  // view into an FBO — get their own planes without leaking state.
  activeFrustumPlanes_ =
      render::Frustum::extractPlanes(camera->projectionMatrix() *
                                     camera->viewMatrix());
  activeFrustumValid_ = true;

  render(_root, camera);

  activeFrustumValid_ = false;

  // Publish stats. Copy rather than move so `currentRenderStats_` is ready
  // for the next pass without another zero-init.
  lastRenderStats_ = currentRenderStats_;

  for (auto light : lights_)
	light->render(camera, lightShader_);

  if (debug_) {
	reactphysics3d::DebugRenderer &debugRenderer = physics_world_->getDebugRenderer();
	auto lines = debugRenderer.getLines();

	for (auto line : lines) {
	  OMEGA_LOG_TRACE("scene", "Debug line: ({}, {}, {}) -> ({}, {}, {})",
					  line.point1.x, line.point1.y, line.point1.z,
					  line.point2.x, line.point2.y, line.point2.z);
	}
  }

  // Leave clipping disabled on exit so callers that don't set it (e.g. the
  // main scene pass right after a portal pass) get unclipped rendering.
  glDisable(GL_CLIP_DISTANCE0);
}

void Scene::render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera) {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	// Every visited object counts as "considered" regardless of whether it
	// survives culling — gives a meaningful ratio in the stats dump.
	++currentRenderStats_.considered;

	// Per-object frustum cull. We deliberately keep the cull conservative:
	//   * Objects that have not been given a bounding sphere (e.g. SkyBox,
	//     loaded sub-meshes that bypass the mesh generator) are never
	//     culled — drawing them is the safe default. Tracked as
	//     `drawnNoBounds` so perf reports can surface objects that would
	//     benefit from having a sphere set.
	//   * The sphere is sized in `Object::worldBoundingSphere` to enclose
	//     the geometry under the object's current model matrix, including
	//     non-uniform scale.
	// We only consult the frustum for the main render pass; the per-portal
	// pass does its own bounds check inside PortalRenderer where the virtual
	// camera's frustum is what matters.
	bool hadBounds = false;
	if (activeFrustumValid_) {
	  if (auto worldSphere = object->worldBoundingSphere()) {
		hadBounds = true;
		if (render::Frustum::sphereOutsideAnyPlane(
				activeFrustumPlanes_, worldSphere->center,
				worldSphere->radius)) {
		  ++currentRenderStats_.culledFrustum;
		  continue;
		}
	  }
	}
	if (!hadBounds) ++currentRenderStats_.drawnNoBounds;

	// Push frame-wide clipping uniforms onto this object's bound shader
	// before it renders. Safe to set on any core.vs-compatible shader; the
	// vertex shader ignores the plane when enableClipping is false.
	if (auto shader = object->shader()) {
	  shader->setVec4("clippingPlane", clippingPlane_);
	  shader->setInt("enableClipping", clippingEnabled_ ? 1 : 0);
	}
	++currentRenderStats_.drawn;
	object->render(camera);
  }

  for (auto child : node->children)
	render(child, camera);
}

// draws the model, and thus all its meshes
void Scene::shaders(std::shared_ptr<render::Shader> shader,
					std::shared_ptr<render::Shader> lightShader) {
  shaders(shader, _root);

  meshShader_ = shader;
  lightShader_ = lightShader;
}

void Scene::shaders(std::shared_ptr<render::Shader> shader, ObjectNodePtr node) {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	object->setShader(shader);
  }

  for (auto child : node->children)
	shaders(shader, child);
}

void Scene::lights(std::vector<std::shared_ptr<Light>> light_list) {
  lights_ = light_list;

  lights(light_list, _root);
}

void Scene::lights(std::vector<std::shared_ptr<Light>> light_list, ObjectNodePtr node) {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	object->affectedByLights(light_list);
  }

  for (auto child : node->children)
	lights(light_list, child);
}

// function to find an object by name and return it
auto Scene::object(std::string name) -> std::shared_ptr<Object> {
  return object( name, _root);
}

auto Scene::object(std::string name, ObjectNodePtr node) -> std::shared_ptr<Object> {
  if (node == nullptr)
	return nullptr;

  for (auto object : node->meshes) {
	if (object->name()==name)
	  return object;
  }

  for (auto child : node->children) {
	auto obj = object(name, child);
	if (obj)
	  return obj;
  }

  return nullptr;
}

// function to find an object by name and return it
auto Scene::scale(float scale) -> void {
//  for (auto object : meshes)   // TODO
//	object->scale(scale);
}

auto Scene::process(float deltaTime) -> void {
  if (debug_) {
	reactphysics3d::DebugRenderer &debugRenderer = physics_world_->getDebugRenderer();

	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::CONTACT_NORMAL, true);
  }

  if (deltaTime==0)
	deltaTime = 0.000001f;
  physics_world_->update(deltaTime);

  // Feed the perf-log accumulator with the same dt that drove physics. We
  // read the raw argument (not the zero-guarded value above) so the log
  // window matches wall-clock time rather than the clamp.
  if (perfLoggingEnabled_) {
    perfAccumSeconds_ += deltaTime;
  }

  process(_root);
}

auto Scene::process(ObjectNodePtr node) -> void {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	object->process();
  }

  for (auto child : node->children)
	process(child);
}

auto Scene::setCurrentCamera(unsigned int index) -> void {
  auto camera = cameras_[index];

  camera->setupPhysics(physics_world_, &physics_common_);

  current_camera_ = index;
}

