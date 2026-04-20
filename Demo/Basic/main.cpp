#include <iostream>
#include <filesystem>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <geometry/Point3.h>
#include <render/Window.h>
#include <system/System.h>
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

  // Load a model once and drop it into the scene at the given pose.
  // Rotation is a yaw in radians (around Y). Returns true on success.
  bool placeModel(const std::string& path,
                  const glm::vec3& position,
                  float yawRadians,
                  float scale) {
	auto tree = Loader::loadModel(path);
	if (!tree) return false;

	auto world = glm::mat4(1.0f);
	world = glm::translate(world, position);
	world = glm::rotate(world, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	world = glm::scale(world, glm::vec3(scale));

	applyTransformAndShader(tree, world, shader);
	_scene->add(tree);
	return true;
  }

  // Scatter trees and rocks around the ground plane. Placement avoids the
  // central playground (containers + cubes) and stays inside the 50×50 ground
  // (size=25 half-extent). Trees come in small clusters to read as natural
  // copses; a few rocks live at cluster bases with others scattered between.
  void generateForest() {
	// Deterministic RNG — we want the layout reproducible run-to-run so you
	// can tune it; swap seed for std::random_device if you prefer variance.
	std::mt19937 rng(1337);
	std::uniform_real_distribution<float> jitter(-2.5f, 2.5f);
	std::uniform_real_distribution<float> yaw(0.0f, glm::two_pi<float>());
	std::uniform_real_distribution<float> treeScale(0.7f, 1.25f);
	std::uniform_real_distribution<float> rockScale(0.35f, 0.95f);
	std::uniform_int_distribution<int> treeIdx(0, 4);
	std::uniform_int_distribution<int> rockIdx(0, 2);

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

	// Copse centers — picked around the perimeter so the foreground (where
	// the cubes/containers sit) stays readable. Each cluster spawns 3-5 trees.
	struct Copse { glm::vec3 center; int count; };
	const std::array<Copse, 5> copses = {{
		{ { -18.0f, 0.0f, -17.0f }, 5 },
		{ {  18.0f, 0.0f, -14.0f }, 4 },
		{ { -20.0f, 0.0f,  10.0f }, 4 },
		{ {  16.0f, 0.0f,  18.0f }, 5 },
		{ {   0.0f, 0.0f, -22.0f }, 3 },
	}};

	for (const auto& copse : copses) {
	  for (int i = 0; i < copse.count; ++i) {
		glm::vec3 pos = copse.center + glm::vec3(jitter(rng), 0.0f, jitter(rng));
		// Clamp away from the playground (r < 5 near origin) so we don't
		// end up with a tree sprouting between the containers.
		if (glm::length(glm::vec2(pos.x, pos.z)) < 5.0f) continue;
		placeModel(trees[treeIdx(rng)], pos, yaw(rng), treeScale(rng));
	  }

	  // Drop a rock or two at the base of each cluster for grounding.
	  for (int r = 0; r < 2; ++r) {
		glm::vec3 pos = copse.center + glm::vec3(jitter(rng) * 0.5f, 0.0f,
		                                         jitter(rng) * 0.5f);
		placeModel(rocks[rockIdx(rng)], pos, yaw(rng), rockScale(rng));
	  }
	}

	// A few lone trees to break up the copse rhythm.
	const std::array<glm::vec3, 4> lonely = {{
		{ -12.0f, 0.0f,  20.0f },
		{  22.0f, 0.0f,   4.0f },
		{ -22.0f, 0.0f,  -4.0f },
		{   8.0f, 0.0f, -20.0f },
	}};
	for (const auto& p : lonely) {
	  placeModel(trees[treeIdx(rng)], p, yaw(rng), treeScale(rng));
	}

	// Scattered rocks between the copses.
	std::uniform_real_distribution<float> ground(-22.0f, 22.0f);
	for (int i = 0; i < 6; ++i) {
	  glm::vec3 pos(ground(rng), 0.0f, ground(rng));
	  if (glm::length(glm::vec2(pos.x, pos.z)) < 6.0f) { --i; continue; }
	  placeModel(rocks[rockIdx(rng)], pos, yaw(rng), rockScale(rng));
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
										   .size = 25.f,
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
	return Window::render();
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
