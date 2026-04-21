#include <iostream>
#include <filesystem>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <geometry/Point3.h>
#include <render/Window.h>
#include <system/System.h>
#include <system/Log.h>
#include <render/CameraFPS.h>
#include <render/Shader.h>
#include <render/Material.h>
#include <render/Texture.h>
#include <geometry/Object.h>
#include <system/FileSystem.h>
#include <geometry/Scene.h>

#include <render/DirectionalLight.h>
#include <render/PointLight.h>
#include <render/SpotLight.h>

#include <utils/ObjectGenerator.h>
#include <utils/Loader.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <random>
#include <unordered_map>

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::utils;
using namespace omega::interface;
using namespace omega::input;
using namespace omega;

class MainWindow : public Window {
public:
  MainWindow() : Window() {
	// Get the executable directory
	std::filesystem::path exePath;
	#ifdef __APPLE__
		// On macOS, use _NSGetExecutablePath
		uint32_t size = 0;
		_NSGetExecutablePath(nullptr, &size);
		std::vector<char> path(size);
		_NSGetExecutablePath(path.data(), &size);
		exePath = std::filesystem::canonical(path.data()).parent_path();
	#else
		// On Linux, use /proc/self/exe
		exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
	#endif
	
	// Look for resources.zip in the executable directory
	std::filesystem::path zipPath = exePath / "resources.zip";
	if (!std::filesystem::exists(zipPath)) {
		// Fallback: try Demo directory relative to executable
		zipPath = exePath.parent_path() / "Demo" / "resources.zip";
	}
	
	fs::instance()->add(zipPath.string());

	auto camera = std::make_shared<CameraFPS>(glm::vec3(0.0f, 1.0f, 0.0f),
											  glm::vec3(0.0f, 2.0f, 0.0f), -110.f);
	shader = Shader::fromFile(4,
							  2,
							  ":/shaders/core.vs",
							  ":/shaders/core.fs");
	shader->setInt("texture1", 0);
	shader->setVec4("ambient", 0.15f, 0.15f, 0.15f, 1.0f);

	plainShader =
		Shader::fromFile(4, 2, ":/shaders/plain.vs", ":/shaders/plain.fs");
	plainShader->setInt("texture1", 0);

	skyShader =
		Shader::fromFile(4, 2, ":/shaders/skybox.vs", ":/shaders/skybox.fs");
	skyShader->setInt("skybox", 0);

	texture1 = std::make_shared<Texture>();
	texture1->load(":/textures/Cargo_container_v1.tga");

	texture2 = std::make_shared<Texture>();
	texture2->load(":/textures/container2_specular.png");

	texture3 = std::make_shared<Texture>();
	texture3->load(":/textures/pbr/grass/albedo.png");

	texture4 = std::make_shared<Texture>();
	texture4->load(":/textures/container2.png");

/*
	_scene = std::make_shared<Scene>("/Users/cta/Development/personal/Omega/Demo/Resources/models/big_map.fbx");
	_scene->shaders(shader, plainShader);
	_scene->lights(lights_);
	_scene->scale(0.2f);
*/
	_scene = std::make_shared<Scene>(false);
	_scene->shaders(shader, plainShader);
	_scene->debug(false);

//	_scene->import("./resources/models/mp7.fbx");

	auto idx = _scene->add(camera);

	createLights();
	generateCubes();
	generateContainer();
	generateGround();
	generateForest();
	generateDome();

	_scene->prepare();
	_scene->setCurrentCamera(idx);

	setCamera(camera);
  }

  void generateDome() {
	auto skyBox =
		ObjectGenerator::dome({.front = ":/textures/skybox/front.jpg",
								  .back = ":/textures/skybox/back.jpg",
								  .left = ":/textures/skybox/left.jpg",
								  .right = ":/textures/skybox/right.jpg",
								  .top = ":/textures/skybox/top.jpg",
								  .bottom = ":/textures/skybox/bottom.jpg"});
	skyBox->setShader(skyShader);
	_scene->add(skyBox);
  }

  void createLights() {
	auto dir_light = std::make_shared<DirectionalLight>(DirectionalLightInput{
		.direction = glm::vec3(-0.2f, 1.0f, -0.3f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
		.specular = glm::vec3(0.5f, 0.5f, 0.5f),
	});

	auto point_light1 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(0.7f, 1.0f, 2.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.3f, 0.8f, 0.8f),
		.specular = glm::vec3(0.5f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto point_light2 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(2.3f, 3.3f, -4.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.8f, 0.2f, 0.2f),
		.specular = glm::vec3(1.0f, 0.6f, 0.6f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto point_light3 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(-4.0f, 2.0f, -12.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.3f, 0.8f, 0.4f),
		.specular = glm::vec3(0.3f, 1.0f, 0.3f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto point_light4 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(0.0f, 7.0f, -3.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
		.specular = glm::vec3(1.0f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto spot_light = std::make_shared<SpotLight>(
		SpotLightInput{.tracking = _scene->currentCamera(),
			.ambient = glm::vec3(0.0f, 0.0f, 0.0f),
			.diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
			.cutOff = glm::cos(glm::radians(12.5f)),
			.outerCutOff = glm::cos(glm::radians(15.0f))});

	//_scene->add(dir_light);
	_scene->add(point_light1);
	_scene->add(point_light2);
	_scene->add(point_light3);
	_scene->add(point_light4);
	_scene->add(spot_light);
  }

  void generateCubes() {
	glm::vec3 cubePositions[] = {
		glm::vec3(14.0f, 1.0f, 0.0f), glm::vec3(12.0f, 1.0f, -15.0f),
		glm::vec3(-3.0f, 1.0f, -5.0f), glm::vec3(-13.8f, 1.0f, -12.3f),
		glm::vec3(9.4f, 1.f, -7.0f), glm::vec3(-10.7f, 1.0f, -7.5f),
		glm::vec3(4.3f, 1.0f, -5.0f), glm::vec3(8.5f, 1.0f, -12.5f),
		glm::vec3(7.5f, 1.0f, -4.0f), glm::vec3(-9.3f, 1.0f, -10.5f)};

	int nr = 0;
	for (auto pos : cubePositions) {
//	  float angle = 20.0f*(int)(rand()%20);
	  float size = 0.1f*((float)(rand()%6) + 0.1f);
	  auto mat = glm::mat4(1.0f);
	  auto material = Material{.shininess = (float)(rand()%80)};

	  mat = glm::translate(mat, pos);
//	  mat = glm::rotate(mat, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

	  _scene->add(ObjectGenerator::box({.matrix = mat,
										   .shader = shader,
										   .textures = {texture4},
										   .material = material,
										   .size = size,
										   .name = "Cube" + std::to_string(nr++)}));
	}

  }

  // Walk a loaded ObjectNode tree, stamping a world-space model matrix and
  // our lit shader onto every mesh. glb/fbx models come out of the loader
  // with identity model_ on each Object, and Scene doesn't read ObjectNode::mat
  // at render time — so the per-instance transform has to live on each mesh.
  void applyTransformAndShader(const ObjectNodePtr& node,
                                const glm::mat4& world,
                                const std::shared_ptr<Shader>& sh) {
	if (!node) return;
	for (auto& mesh : node->meshes) {
	  mesh->setModel(world);
	  mesh->setShader(sh);
	}
	for (auto& child : node->children) {
	  applyTransformAndShader(child, world, sh);
	}
  }

  // Cache of parsed GLB/FBX prototypes — reused as the geometry template for
  // every instance of a given path, so thousands of trees don't parse the
  // same glb thousands of times.
  std::unordered_map<std::string, ObjectNodePtr> _modelCache;

  // Clone an Object while sharing its GL handles (VAO/VBO) and texture list.
  // Safe because Object has no destructor that would free the GL handles —
  // multiple owners of the same VAO is well-defined as long as no one tries
  // to delete it. Each clone gets its own transform, shader, material.
  //
  // Crucially, we also copy the local-space bounding sphere from the prototype.
  // Without this, every cloned tree/rock would register as "no bounds" at
  // Scene::render time and be drawn unconditionally — effectively disabling
  // frustum culling for the entire forest. The sphere is in local space so it
  // remains valid for any per-instance transform we apply afterwards.
  std::shared_ptr<Object> cloneMesh(const std::shared_ptr<Object>& proto) {
	auto copy = std::make_shared<Object>(proto->getVAO(), proto->getVBO(),
	                                     proto->getCount(), proto->getType());
	copy->setName(proto->name());
	copy->setTextures(proto->getTextures());
	if (auto mat = proto->getMaterial()) copy->setMaterial(*mat);
	if (const auto& sphere = proto->boundingSphere()) {
	  copy->setBoundingSphere(*sphere);
	}
	return copy;
  }

  // Deep-copy an ObjectNode tree, cloning each mesh but keeping shared GL
  // handles. The returned tree is independent so transforms/shaders set on
  // it don't bleed back into the prototype or other instances.
  ObjectNodePtr cloneNode(const ObjectNodePtr& node) {
	if (!node) return nullptr;
	auto out = std::make_shared<ObjectNode>();
	out->mat = node->mat;
	out->meshes.reserve(node->meshes.size());
	for (const auto& m : node->meshes) out->meshes.push_back(cloneMesh(m));
	out->children.reserve(node->children.size());
	for (const auto& c : node->children) out->children.push_back(cloneNode(c));
	return out;
  }

  // Load the model once, cache it, and return the prototype tree.
  ObjectNodePtr cachedLoad(const std::string& path) {
	auto it = _modelCache.find(path);
	if (it != _modelCache.end()) return it->second;
	auto tree = Loader::loadModel(path);
	if (tree) _modelCache[path] = tree;
	return tree;
  }

  // Place an instance of a (cached) model at the given pose. Each call allocates
  // lightweight Object wrappers that share the prototype's VAO but carry an
  // independent per-instance model matrix.
  bool placeModel(const std::string& path,
                  const glm::vec3& position,
                  float yawRadians,
                  float scale) {
	auto proto = cachedLoad(path);
	if (!proto) return false;
	auto instance = cloneNode(proto);
	if (!instance) return false;

	auto world = glm::mat4(1.0f);
	world = glm::translate(world, position);
	world = glm::rotate(world, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	world = glm::scale(world, glm::vec3(scale));

	applyTransformAndShader(instance, world, shader);
	_scene->add(instance);
	return true;
  }

  // Scatter trees and rocks across the ground plane at ~10× the previous
  // density. Previous pass produced ~190 placements; we now aim for ~1900 —
  // dense enough that the camera sees overlapping canopy from any angle
  // without hitting frame-budget trouble because every instance shares its
  // VAO/VBO with the cached prototype (see _modelCache + cloneMesh).
  //
  // Structure:
  //   • Procedurally generated ring of ~80 copses around the playground,
  //     each dropping 6–12 trees (~640 trees).
  //   • Dense undergrowth/sapling scatter across the full ring (~900).
  //   • Scattered rocks at cluster bases + across the ground (~350).
  //   • Handful of lone feature trees to break rhythm (~24).
  // Ground is size=60 (half-extent 60), so we cap placements at 58 to keep
  // trunks from clipping the ground edge.
  void generateForest() {
	// Deterministic RNG — reproducible run-to-run so tuning is predictable.
	std::mt19937 rng(1337);
	std::uniform_real_distribution<float> copseJitter(-3.5f, 3.5f);
	std::uniform_real_distribution<float> tightJitter(-1.8f, 1.8f);
	std::uniform_real_distribution<float> yaw(0.0f, glm::two_pi<float>());
	std::uniform_real_distribution<float> treeScale(0.65f, 1.45f);
	std::uniform_real_distribution<float> saplingScale(0.45f, 0.9f);
	std::uniform_real_distribution<float> rockScale(0.3f, 1.1f);
	std::uniform_int_distribution<int> treeIdx(0, 4);
	std::uniform_int_distribution<int> rockIdx(0, 2);
	std::uniform_int_distribution<int> copseSize(6, 12);

	// Minimum distance from origin where a tree may spawn. The containers/
	// cubes playground sits within r ≈ 4; a small buffer keeps gameplay clear.
	const float playgroundRadius = 5.0f;
	const float groundExtent = 58.0f;   // keep trunks inside the 60 half-extent

	const std::array<const char*, 5> trees = {
		":/models/tree.glb",
		":/models/tree-high.glb",
		":/models/tree-crooked.glb",
		":/models/tree-high-crooked.glb",
		":/models/tree-high-round.glb",
	};
	const std::array<const char*, 3> rocks = {
		":/models/rock-small.glb",
		":/models/rock-wide.glb",
		":/models/rock-large.glb",
	};

	auto insideBounds = [&](const glm::vec3& p) {
	  if (std::abs(p.x) > groundExtent || std::abs(p.z) > groundExtent) return false;
	  if (glm::length(glm::vec2(p.x, p.z)) < playgroundRadius) return false;
	  return true;
	};

	auto tryTree = [&](const glm::vec3& p, float scale) {
	  if (!insideBounds(p)) return;
	  placeModel(trees[treeIdx(rng)], p, yaw(rng), scale);
	};
	auto tryRock = [&](const glm::vec3& p, float scaleMul = 1.0f) {
	  if (!insideBounds(p)) return;
	  placeModel(rocks[rockIdx(rng)], p, yaw(rng), rockScale(rng) * scaleMul);
	};

	// Procedural copse centers. Four concentric rings around the playground
	// with jittered angular placement — uniform enough to cover the ground,
	// jittered enough that it doesn't read as a grid.
	std::uniform_real_distribution<float> ringJitter(-2.5f, 2.5f);
	std::uniform_real_distribution<float> angJitter(-0.15f, 0.15f);
	struct Ring { float radius; int count; };
	const std::array<Ring, 4> rings = {{
		{ 12.0f, 14 },
		{ 24.0f, 20 },
		{ 38.0f, 24 },
		{ 52.0f, 22 },
	}};

	int copseCount = 0;
	for (const auto& r : rings) {
	  for (int i = 0; i < r.count; ++i) {
		const float baseAng = glm::two_pi<float>() * float(i) / float(r.count);
		const float ang = baseAng + angJitter(rng);
		const float radius = r.radius + ringJitter(rng);
		glm::vec3 center(std::cos(ang) * radius, 0.0f, std::sin(ang) * radius);

		const int count = copseSize(rng);
		for (int t = 0; t < count; ++t) {
		  tryTree(center + glm::vec3(copseJitter(rng), 0.0f, copseJitter(rng)),
				  treeScale(rng));
		}
		// Rocks at copse bases. More on outer rings where the ground is
		// otherwise emptier.
		const int rocksPerCopse = (r.radius > 30.0f) ? 4 : 2;
		for (int k = 0; k < rocksPerCopse; ++k) {
		  tryRock(center + glm::vec3(tightJitter(rng), 0.0f, tightJitter(rng)));
		}
		++copseCount;
	  }
	}

	// Dense undergrowth/sapling scatter. Full-plane uniform scatter; reject
	// the playground core with a slightly larger buffer so saplings don't
	// creep into the gameplay zone.
	std::uniform_real_distribution<float> plane(-groundExtent, groundExtent);
	const int saplingAttempts = 1200;  // ~900 succeed after bounds rejection
	for (int i = 0; i < saplingAttempts; ++i) {
	  glm::vec3 pos(plane(rng), 0.0f, plane(rng));
	  if (!insideBounds(pos)) continue;
	  if (glm::length(glm::vec2(pos.x, pos.z)) < playgroundRadius + 1.5f) continue;
	  placeModel(trees[treeIdx(rng)], pos, yaw(rng), saplingScale(rng));
	}

	// Rocks scattered throughout — ground-floor detail, independent of copses.
	const int rockAttempts = 420;
	for (int i = 0; i < rockAttempts; ++i) {
	  glm::vec3 pos(plane(rng), 0.0f, plane(rng));
	  if (!insideBounds(pos)) continue;
	  placeModel(rocks[rockIdx(rng)], pos, yaw(rng), rockScale(rng));
	}

	// Lone feature trees at full scale, scattered to break the rhythm of the
	// rings. Positions are generated rather than hand-placed because the
	// ground is now too large to hand-tune 24 points usefully.
	const int lonelyCount = 24;
	std::uniform_real_distribution<float> lonelyScale(1.2f, 1.8f);
	for (int i = 0; i < lonelyCount; ++i) {
	  glm::vec3 pos(plane(rng), 0.0f, plane(rng));
	  if (!insideBounds(pos)) continue;
	  placeModel(trees[treeIdx(rng)], pos, yaw(rng), lonelyScale(rng));
	}
  }

  void generateGround() {
	auto mat = glm::mat4(1.0f);
	auto material = Material{.shininess = (float)(rand()%80)};

	mat = glm::translate(mat, glm::vec3(0.f, 0.f, 0.f));
	_scene->add(ObjectGenerator::plane({.matrix = mat,
										   .shader = shader,
										   .textures = {texture3},
										   .material = material,
										   .size = 60.f,
										   .name = "Ground"}));
  }

  void generateContainer() {

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(2.0f, 0.5f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .size = 0.5f,
											   .mass = 10000.f,
										   }));

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(-1.0f, 0.5f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .size = 0.5f,
											   .mass = 10000.f,
										   }));

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(1.0f, 1.5f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .size = 0.5f,
											   .mass = 10000.f,
										   }));

  }

  void keyEvent(int state, int key, int modifier, bool repeat) {
	switch (key) {
	case KEY_ESCAPE:
	  if (state==KEY_STATE_DOWN)
		quit();
	  break;
	case KEY_1:
	  if (state==KEY_STATE_DOWN)
		setSize(640, 480);
	  break;
	case KEY_2:
	  if (state==KEY_STATE_DOWN)
		setSize(1024, 768);
	  break;
	case KEY_F:
	  if (state==KEY_STATE_DOWN)
		setFullscreen(not isFullscreen());
	  break;
	case KEY_J:
	  if (state==KEY_STATE_DOWN) {
		auto object = _scene->object("Warehouse1.door1");
		if (object)
		  object->visible(!object->visible());

	  }
	  break;
	case KEY_K:
	  if (state==KEY_STATE_DOWN) {
		auto object = _scene->object("Warehouse1.door2");
		if (object)
		  object->visible(!object->visible());

	  }
	  break;
	default:Window::keyEvent(state, key, modifier, repeat);
	  break;
	}
  }

  void process() {
	_scene->process(m_deltaTime);
	Window::process();
  }

  bool render() {
	_scene->render();
	reportPerformance();
	return Window::render();
  }

  // Roll a 1-second window of per-frame dt values and log FPS + the latest
  // Scene render stats once per second. We deliberately average over wall time
  // rather than a fixed frame count: at 15 fps a 60-frame window is 4 seconds
  // of lag, and at 600 fps it's 100 ms of noise. Accumulating until the dt
  // total crosses 1.0 s keeps the report cadence steady regardless of load.
  void reportPerformance() {
	perfAccumSeconds_ += m_deltaTime;
	++perfFrameCount_;

	if (perfAccumSeconds_ < 1.0f) return;

	const float fps = (perfAccumSeconds_ > 0.f)
	                     ? float(perfFrameCount_) / perfAccumSeconds_
	                     : 0.f;
	const float msPerFrame = (perfFrameCount_ > 0)
	                            ? (perfAccumSeconds_ * 1000.f) / float(perfFrameCount_)
	                            : 0.f;

	const auto& s = _scene->lastRenderStats();
	const int culledPct = (s.considered > 0)
	                         ? int((100.f * s.culledFrustum) / float(s.considered))
	                         : 0;

	// Single-line report: FPS + frame time, then draw/cull/no-bounds of the
	// most recent frame, then a clip flag. Keeping it one line makes it easy
	// to grep in the console stream while the demo is running.
	OMEGA_LOG_INFO(
	    "perf",
	    "FPS={:.1f} ({:.2f} ms) | draw={} cull={} ({}%) nobounds={} "
	    "considered={} clip={}",
	    fps, msPerFrame, s.drawn, s.culledFrustum, culledPct,
	    s.drawnNoBounds, s.considered, s.clippingActive ? "on" : "off");

	perfAccumSeconds_ = 0.f;
	perfFrameCount_ = 0;
  }

private:
  std::shared_ptr<Scene> _scene;

  std::shared_ptr<Shader> skyShader;
  std::shared_ptr<Shader> shader;
  std::shared_ptr<Shader> plainShader;

  std::shared_ptr<Texture> texture1;
  std::shared_ptr<Texture> texture2;
  std::shared_ptr<Texture> texture3;
  std::shared_ptr<Texture> texture4;

  // Perf reporting state — see reportPerformance().
  float perfAccumSeconds_{0.0f};
  int   perfFrameCount_{0};
};

int main(int argc, char* argv[]) {
  OSystem::init();

  auto window = new MainWindow();

  while (window->isRuning()) {
	window->process();
	window->clear();
	window->render();
	window->swap();
  }

  return 0;
}
