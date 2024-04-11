#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <geometry/ObjectTree.h>
#include <system/FileSystem.h>
#include <render/Camera.h>

using namespace std;
using namespace omega::render;
using namespace omega::interface;

ObjectTree::ObjectTree() {
}

void ObjectTree::render(std::shared_ptr<render::Camera> camera) {
}

auto ObjectTree::setupPhysics(reactphysics3d::PhysicsWorld *world, reactphysics3d::PhysicsCommon *common) -> void {
}

auto ObjectTree::process() -> void {
}