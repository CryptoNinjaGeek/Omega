#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <geometry/Scene.h>
#include <geometry/PortalRenderer.h>
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

  render(_root, camera);

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
	// Push frame-wide clipping uniforms onto this object's bound shader
	// before it renders. Safe to set on any core.vs-compatible shader; the
	// vertex shader ignores the plane when enableClipping is false.
	if (auto shader = object->shader()) {
	  shader->setVec4("clippingPlane", clippingPlane_);
	  shader->setInt("enableClipping", clippingEnabled_ ? 1 : 0);
	}
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

