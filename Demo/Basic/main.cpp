#include <iostream>

#include <render/Window.h>
#include <system/System.h>
#include <render/CameraFPS.h>
#include <render/Shader.h>
#include <render/Material.h>
#include <render/OpenGLRender.h>
#include <render/Texture.h>
#include <geometry/Object.h>
#include <geometry/ComplexObject.h>
#include <system/FileSystem.h>
#include <geometry/Scene.h>

#include <render/DirectionalLight.h>
#include <render/PointLight.h>
#include <render/SpotLight.h>

#include <utils/ObjectGenerator.h>

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

	fs::instance()->add(
		"/Users/cta/Development/personal/Omega/Demo/resources.zip");

	auto camera = std::make_shared<CameraFPS>(glm::vec3(5.0f, 1.8f, 5.0f),
											  glm::vec3(0.0f, 4.0f, 0.0f), -90.f);
	shader = Shader::fromFile(4,
							  2,
							  ":/shaders/core.vs",
							  "/Users/cta/Development/personal/Omega/Demo/Resources/shaders/core.fs");
	shader->setInt("texture1", 0);
	shader->setVec4("ambient", 0.15f, 0.15f, 0.15f, 1.0f);

	plainShader =
		Shader::fromFile(4, 2, ":/shaders/plain.vs", ":/shaders/plain.fs");
	plainShader->setInt("texture1", 0);

	skyShader =
		Shader::fromFile(4, 2, ":/shaders/skybox.vs", ":/shaders/skybox.fs");
	skyShader->setInt("skybox", 0);

	colorShader =
		Shader::fromFile(4, 2,
						 "/Users/cta/Development/personal/Omega/Demo/Resources/shaders/color.vs",
						 "/Users/cta/Development/personal/Omega/Demo/Resources/shaders/color.fs");
	colorShader->setInt("texture1", 0);

	texture1 = std::make_shared<Texture>();
	texture1->load(":/textures/Cargo_container_v1.tga");

	texture2 = std::make_shared<Texture>();
	texture2->load(":/textures/container2_specular.png");

	texture3 = std::make_shared<Texture>();
	texture3->load(":/textures/pbr/grass/albedo.png");

	texture4 = std::make_shared<Texture>();
	texture4->load(":/textures/container2.png");

	debugTexture = std::make_shared<Texture>();
	debugTexture->load("/Users/cta/Development/personal/Omega/Demo/Resources/textures/aabb.png");

	OpenGLRender::instance()->debugShader(colorShader);
	OpenGLRender::instance()->debugTexture(debugTexture);

/*
	_scene = std::make_shared<Scene>("/Users/cta/Development/personal/Omega/Demo/Resources/models/big_map.fbx");
	_scene->shaders(shader, plainShader);
	_scene->lights(lights_);
	_scene->scale(0.2f);
*/
	_scene = std::make_shared<Scene>(true);
	_scene->debug(true);
	_scene->import("/Users/cta/Development/personal/Omega/Demo/Resources/models/box.fbx");
//	_scene->import("/Users/cta/Development/personal/Omega/Demo/Resources/models/tank_test_ani.fbx");
//	_scene->import("/Users/cta/Development/personal/Omega/Demo/Resources/models/container.fbx");
	_scene->shaders(shader, plainShader, colorShader);
	_scene->scale(0.003f);
/*
	auto gun = std::make_shared<ComplexObject>();
	gun->load("/Users/cta/Development/personal/Omega/Demo/Resources/models/mp7.fbx", 0.01f);
	gun->setShader(shader);
	gun->translate(glm::vec3(0.088f, 0.376999f, -0.22f));
	gun->rotate(90.f, glm::vec3(1.f, 0.f, 0.f));
	gun->follow(camera);
*/
	//_scene->add(gun);

	auto idx = _scene->add(camera);

	createLights();
//	generateCubes();
//	generateContainer();
	generateGround();
	generateDome();

	_scene->prepare();
	_scene->setCurrentCamera(idx);

//	_currentObject = gun;

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
		SpotLightInput{.follow = _scene->currentCamera(),
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
		glm::vec3(0.0f, 1.0f, -5.0f), glm::vec3(0.0f, 1.0f, -10.0f),
		glm::vec3(-3.0f, 1.0f, -5.0f), glm::vec3(-13.8f, 1.0f, -12.3f),
		glm::vec3(9.4f, 1.f, -7.0f), glm::vec3(-10.7f, 1.0f, -7.5f),
		glm::vec3(4.3f, 1.0f, -5.0f), glm::vec3(8.5f, 1.0f, -12.5f),
		glm::vec3(7.5f, 1.0f, -4.0f), glm::vec3(-9.3f, 1.0f, -10.5f)};

	int nr = 0;
	for (auto pos : cubePositions) {
	  float size = 0.2f*((float)(rand()%6) + 0.2f);
	  auto mat = glm::mat4(1.0f);
	  auto material = Material{.shininess = (float)(rand()%80)};

	  mat = glm::translate(mat, pos);

	  _scene->add(ObjectGenerator::box({.matrix = mat,
										   .shader = shader,
										   .textures = {texture4},
										   .material = material,
										   .size = size,
										   .mass = 1.f*size,
										   .name = "Cube" + std::to_string(nr++)}));
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
										   .size = 50.f,
										   .name = "Ground"}));
  }

  void generateContainer() {

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(3.5f, 1.0f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .mass = 10000.f,
										   }));

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(-3.0f, 1.0f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .mass = 10000.f,
										   }));

	_scene->add(ObjectGenerator::container({
											   .position = glm::vec3(1.0f, 3.0f, 5.0f),
											   .shader = shader,
											   .textures = {texture1},
											   .material = Material{.shininess = (float)(rand()%80)},
											   .mass = 10000.f,
										   }));

  }

  void keyEvent(int state, int key, int modifier, bool repeat) {
	switch (key) {
	case KEY_Y:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(0.0f, 0.1f, 0.0f));
	  break;
	case KEY_U:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(0.0f, -0.1f, 0.0f));
	  break;
	case KEY_H:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(-0.1f, 0.0f, 0.0f));
	  break;
	case KEY_J:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(0.1f, 0.0f, 0.0f));
	  break;
	case KEY_N:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(0.0f, 0.0f, 0.1f));
	  break;
	case KEY_M:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->translate(glm::vec3(0.0f, 0.0f, -0.1f));
	  break;
	case KEY_I:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->rotate(1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	  break;
	case KEY_O:
	  if (state==KEY_STATE_DOWN && _currentObject)
		_currentObject->rotate(-1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	  break;
	case KEY_P:
	  if (state==KEY_STATE_DOWN && _currentObject) {
		auto model = _currentObject->entityModel();
		std::cout << "Model: " << glm::to_string(model) << std::endl;
	  }
	  break;
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

  std::shared_ptr<ObjectInterface> _currentObject{nullptr};

  std::shared_ptr<Shader> skyShader;
  std::shared_ptr<Shader> shader;
  std::shared_ptr<Shader> plainShader;
  std::shared_ptr<Shader> colorShader;

  std::shared_ptr<Texture> debugTexture;

  std::shared_ptr<Texture> texture1;
  std::shared_ptr<Texture> texture2;
  std::shared_ptr<Texture> texture3;
  std::shared_ptr<Texture> texture4;
};

int main() {
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
