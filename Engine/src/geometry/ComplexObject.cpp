#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <geometry/ComplexObject.h>
#include <system/FileSystem.h>
#include <render/Camera.h>

using namespace std;
using namespace omega::render;
using namespace omega::interface;

ComplexObject::ComplexObject() {
}

void ComplexObject::render(std::shared_ptr<render::Camera> camera) {
}

auto ComplexObject::setupPhysics(reactphysics3d::PhysicsWorld *world, reactphysics3d::PhysicsCommon *common) -> void {
}

auto ComplexObject::process() -> void {
}