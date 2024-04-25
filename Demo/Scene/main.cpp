#include <iostream>
#include <iso646.h>

#include <render/Window.h>
#include <system/System.h>
#include <render/Camera.h>
#include <render/Shader.h>
#include <render/Material.h>
#include <render/Texture.h>
#include <render/OpenGLRender.h>
#include <geometry/Object.h>

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

class MainWindow : public Window {
public:
  MainWindow() : Window() {
	camera = std::make_shared<Camera>(glm::vec3(1.0f, 0.0f, 3.0f),
									  glm::vec3(0.0f, 1.0f, 0.0f), -110.f);
	shader = Shader::fromFile(
		4, 2, "/Users/cta/Development/personal/Omega/Demo/Basic/vertex_core.vs",
		"/Users/cta/Development/personal/Omega/Demo/Basic/fragment_core.fs");
	shader->setInt("texture1", 0);
	shader->setVec3("ambient", 0.05f, 0.05f, 0.05f);

	texture1 = std::make_shared<Texture>();
	texture1->load(
		"/Users/cta/Development/personal/Omega/Demo/Basic/container2.jpg");

	texture2 = std::make_shared<Texture>();
	texture2->load(
		"/Users/cta/Development/personal/Omega/Demo/Basic/"
		"container2_specular.png");

	createLights();
	generateCubes();

	setCamera(camera);
  }

  void createLights() {
	auto dir_light = std::make_shared<DirectionalLight>(DirectionalLightInput{
		.direction = glm::vec3(-0.2f, -1.0f, -0.3f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
		.specular = glm::vec3(0.5f, 0.5f, 0.5f),
	});

	auto point_light1 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(0.7f, 0.2f, 2.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.3f, 0.8f, 0.8f),
		.specular = glm::vec3(0.5f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto point_light2 = std::make_shared<PointLight>(PointLightInput{
		.position = glm::vec3(2.3f, -3.3f, -4.0f),
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
		.position = glm::vec3(0.0f, 0.0f, -3.0f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
		.specular = glm::vec3(1.0f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
	});

	auto spot_light = std::make_shared<SpotLight>(
		SpotLightInput{.follow = camera,
			.ambient = glm::vec3(0.0f, 0.0f, 0.0f),
			.diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
			.cutOff = glm::cos(glm::radians(12.5f)),
			.outerCutOff = glm::cos(glm::radians(15.0f))});

	lights_.push_back(dir_light);

	lights_.push_back(point_light1);
	lights_.push_back(point_light2);
	lights_.push_back(point_light3);
	lights_.push_back(point_light4);

	lights_.push_back(spot_light);
  }

  void generateCubes() {
	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f), glm::vec3(-1.7f, 3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f), glm::vec3(1.5f, 2.0f, -2.5f),
		glm::vec3(1.5f, 0.2f, -1.5f), glm::vec3(-1.3f, 1.0f, -1.5f)};

	for (auto pos : cubePositions) {
	  float angle = 20.0f*(int)(rand()%20);
	  auto mat = glm::mat4(1.0f);
	  auto material = Material{.shininess = (float)(rand()%80)};

	  mat = glm::translate(mat, pos);
	  mat = glm::rotate(mat, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

	  object_list_.push_back(ObjectGenerator::box({.matrix = mat,
													  .shader = shader,
													  .textures = {texture1},
													  .material = material,
													  .lights = lights_}));
	}
  }

  void keyEvent(int type, int state, int key, bool repeat) {
	switch (key) {
	case KEY_ESCAPE:
	  if (state==KEY_STATE_DOWN)
		quit();
	  break;
	case KEY_F:
	  if (state==KEY_STATE_DOWN)
		setFullscreen(not isFullscreen());
	  break;
	default:Window::keyEvent(type, state, key, repeat);
	  break;
	}
  }

  bool render() {
	for (auto object : object_list_)
	  OpenGLRender::instance()->render(object);
	return Window::render();
  }

private:
  std::shared_ptr<Shader> shader;
  std::shared_ptr<Camera> camera;
  std::shared_ptr<Texture> texture1;
  std::shared_ptr<Texture> texture2;
  std::vector<std::shared_ptr<Object>> object_list_;
  std::vector<std::shared_ptr<Light>> lights_;
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
