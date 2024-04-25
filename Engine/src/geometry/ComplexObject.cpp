#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <geometry/ComplexObject.h>
#include <system/FileSystem.h>
#include <render/Camera.h>
#include <utils/Loader.h>

using namespace std;
using namespace omega::render;
using namespace omega::utils;
using namespace omega::interface;

ComplexObject::ComplexObject(bool debug) : debug_(debug) {
  data_->type_ = ObjectType::ComplexObject;
}

void ComplexObject::load(string const &path, double scale) {
  _root = Loader::loadModel({
								.path = path,
								.scale = scale,
								.debug = debug_
							});
  _root->mat = glm::mat4(1.0f);
}

auto ComplexObject::process() -> void {
  process(_root);
}

auto ComplexObject::process(ObjectNodePtr node) -> void {
  if (node==nullptr)
	return;

  for (auto object : node->meshes) {
	object->process();
  }

  for (auto child : node->children)
	process(child);
}

auto ComplexObject::prepare(input::ObjectPreparation input) -> void {
  prepare(_root, input);
}

auto ComplexObject::prepare(ObjectNodePtr node, input::ObjectPreparation input) -> void {
  if (node==nullptr)
	return;

  for (auto object : node->meshes) {
	object->prepare(input);
  }

  for (auto child : node->children)
	prepare(child, input);
}

auto ComplexObject::setShader(std::shared_ptr<render::Shader> shader) -> void {
  shaders(_root, shader);
}

void ComplexObject::shaders(ObjectNodePtr node, std::shared_ptr<render::Shader> shader) {
  if (node==nullptr)
	return;

  for (auto object : node->meshes) {
	object->setShader(shader);
  }

  for (auto child : node->children)
	shaders(child, shader);
}

void ComplexObject::affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) {
}

auto ComplexObject::scale(float value) -> void {
  _root->mat = glm::scale(_root->mat, glm::vec3(value));
}

auto ComplexObject::rotate(float angle, glm::vec3 axis) -> void {
  _root->mat = glm::rotate(_root->mat, glm::radians(angle), axis);
}

auto ComplexObject::translate(glm::vec3 pos) -> void {
  _root->mat = glm::translate(_root->mat, pos);
}

auto ComplexObject::entityModel() -> glm::mat4 {
  if (_root==nullptr)
	return Entity::entityModel();
  return _root->mat;
}
