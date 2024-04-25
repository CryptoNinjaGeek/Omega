#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <geometry/Scene.h>
#include <system/FileSystem.h>
#include <system/TextureManager.h>
#include <utils/Loader.h>
#include <render/Camera.h>
#include <render/OpenGLRender.h>

using namespace std;
using namespace omega::render;
using namespace omega::system;
using namespace omega::interface;
using namespace omega::utils;
using namespace omega::physics;

Scene::Scene(bool physics, bool gamma) : gammaCorrection_(gamma), physics_(physics) {
  if(physics_){
	auto engine = PhysicsEngine::instance();
	engine->enabled(true);
	engine->gravity(glm::vec3(0, -9.81f, 0));
  }
}

Scene::Scene(std::string const &path, bool physics, bool gamma) : gammaCorrection_(gamma) , physics_(physics) {
  if(physics_){
	auto gravity =PhysicsEngine::instance()->gravity();
	std::cout << "Gravity: " << gravity.x << " " << gravity.y << " " << gravity.z
			  << std::endl;
  }
  loadModel(path);
}

auto Scene::import(std::string const &path) -> void {
  loadModel(path);
  if(meshShader_ && lightShader_)
	  shaders(meshShader_, lightShader_, debugShader_);
}

auto Scene::debug(bool val) -> void {
  debug_ = val;

  if(PhysicsEngine::instance()->enabled()){
	auto texture = std::make_shared<Texture>();
	texture->load("/Users/cta/Development/personal/Omega/Demo/Resources/textures/aabb.png");

	aabb_ = ObjectGenerator::box({.matrix = glm::mat4(1.0f),
								  .shader = debugShader_,
								  .textures = {texture},
								  .size = 0.5f,
								  .mass = 1.0f,
								  .name = "AABB_CUBE",
								  .physics = false,
								  .lighting = false});
  } else {
	aabb_->visible(false);
  }
}

auto Scene::add(std::shared_ptr<ObjectInterface> object) -> void
{
  if(_root ==nullptr)
	_root = std::make_shared<ObjectNode>();

  _root->meshes.push_back(object);
}

void Scene::loadModel(string const &path) {
  auto tree = Loader::loadModel({
	.path = path,
	.debug = debug_,
  });

  add(tree);
}

auto Scene::add(std::shared_ptr<ObjectNode> tree) ->void
{
  if(_root ==nullptr){
	_root = std::make_shared<ObjectNode>();
	_root->mat = glm::mat4(1.0f);
  }

  _root->children.push_back(tree);
}

auto Scene::prepare() -> void {
  prepare(_root);
}

auto Scene::prepare(ObjectNodePtr node) -> void {
  if (node == nullptr)
	return;

  for (auto object : node->meshes) {
	object->prepare({
	  .lights = lights_,
	  .model = node->mat,
	});
  }

  for (auto child : node->children)
	prepare(child);
}

void Scene::render() {
  this->render(cameras_[current_camera_]);
}

// draws the model, and thus all its meshes
void Scene::render(std::shared_ptr<render::Camera> camera) {
  render(_root, camera, _root->mat);

  for (auto light : lights_)
	OpenGLRender::instance()->render(light,lightShader_);

  auto engine = PhysicsEngine::instance();
  if (debug_ && engine->enabled()) {
	for( unsigned int no = 0 ; no < engine->getNbBodies(); no++ ) {
	  auto body = engine->body(no);
	  OpenGLRender::instance()->render(body->aabb());
	}
  }
 }

void Scene::render(ObjectNodePtr node, std::shared_ptr<render::Camera> camera, glm::mat4 mat) {
  if (node == nullptr)
	return;

  auto ogl_render = OpenGLRender::instance();

  for (auto object : node->meshes) {
	ogl_render->push(mat);
	ogl_render->render(object);
	ogl_render->pop();
  }

  for (auto child : node->children){
	auto changed_mat = mat*child->mat;
	render(child, camera, changed_mat);
  }
}

// draws the model, and thus all its meshes
void Scene::shaders(std::shared_ptr<render::Shader> shader,
					std::shared_ptr<render::Shader> lightShader,
					std::shared_ptr<render::Shader> debugShader) {
  shaders(shader, _root);

  if(aabb_)
	aabb_->setShader(debugShader);

  debugShader_	= debugShader;
  meshShader_ 	= shader;
  lightShader_ 	= lightShader;
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
auto Scene::findFirst(std::string name, FindPattern pattern ) -> std::shared_ptr<ObjectInterface> {
  return findFirst( name , _root, pattern);
}

auto Scene::findFirst(std::string name, ObjectNodePtr node, FindPattern pattern) -> std::shared_ptr<ObjectInterface> {
  if (node == nullptr)
	return nullptr;

  for (auto object : node->meshes) {
	if ( ( object->name() == name && pattern == FindPattern::Exact ) ||
		 ( object->name().find(name) != std::string::npos && pattern == FindPattern::Contains ) ||
		 ( object->name().find(name) == 0 && pattern == FindPattern::StartsWith ) )
	  return object;
  }

  for (auto child : node->children) {
	auto obj = findFirst(name, child, pattern);
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
  if (deltaTime==0)
	deltaTime = 0.000001f;

  if(PhysicsEngine::instance()->enabled())
	PhysicsEngine::instance()->update(deltaTime);

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

  if(OpenGLRender::instance())
	OpenGLRender::instance()->camera(camera);

  if(PhysicsEngine::instance()->enabled())
	  camera->setupPhysics();

  current_camera_ = index;
}

auto Scene::find(std::string name, FindPattern pattern ) -> std::vector<std::shared_ptr<ObjectInterface>> {
  return find( name , _root, pattern);
}

auto Scene::find(std::string name, ObjectNodePtr node, FindPattern pattern) -> std::vector<std::shared_ptr<ObjectInterface>> {
  if (node == nullptr)
	return {};

  std::vector<std::shared_ptr<ObjectInterface>> objects;

  for (auto object : node->meshes) {
	if ( ( object->name() == name && pattern == FindPattern::Exact ) ||
		( object->name().find(name) != std::string::npos && pattern == FindPattern::Contains ) ||
		( object->name().find(name) == 0 && pattern == FindPattern::StartsWith ) )
	  objects.push_back( object );
  }

  for (auto child : node->children) {
	auto obj_list = find(name, child, pattern);
	objects.insert(objects.end(), obj_list.begin(), obj_list.end());
  }

  return objects;
}
