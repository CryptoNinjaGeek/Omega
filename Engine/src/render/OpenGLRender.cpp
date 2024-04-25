#include <render/OpenGLRender.h>
#include <render/Shader.h>
#include <render/Camera.h>
#include <render/Texture.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::render;

static std::shared_ptr<OpenGLRender> _manager;

std::shared_ptr<OpenGLRender> OpenGLRender::instance() {
  if (!_manager) {
	_manager = std::make_shared<OpenGLRender>();
  }
  return _manager;
}

auto OpenGLRender::render(std::shared_ptr<geometry::ObjectInterface> object_interface) -> void {
  if (!object_interface->visible())
	return;

  switch (object_interface->type()) {
  case ObjectType::Object: {
	auto object = std::dynamic_pointer_cast<geometry::Object>(object_interface);
	render(object);
  }
	break;
  case ObjectType::ComplexObject: {
	auto complex_object = std::dynamic_pointer_cast<geometry::ComplexObject>(object_interface);
	render(complex_object);
  }
	break;
  case ObjectType::SkyBox: {
	auto skybox = std::dynamic_pointer_cast<geometry::SkyBox>(object_interface);
	render(skybox);
  }
	break;
  }
}

auto OpenGLRender::render(std::shared_ptr<geometry::ComplexObject> object) -> void {
  auto root = object->root();

  if (root) {
	push(root->mat);
	render(root);
	pop();
  }
}

void OpenGLRender::render(ObjectNodePtr node) {
  if (node==nullptr)
	return;

  for (auto object : node->meshes) {
	render(object);
  }

  for (auto child : node->children) {
	push(child->mat);
	render(child);
	pop();
  }
}

auto OpenGLRender::render(std::shared_ptr<geometry::Object> object) -> void {
  if (!object->visible())
	return;

  auto object_data = object->data();

  if (!object_data)
	return;

  if (!object_data->shader_) {
	std::cout << "No shader set for object: " << object->name() << std::endl;
	return;
  }

  glm::mat4 combined_mat = glm::mat4(1.0f);

  for (auto mat : stack_)
	combined_mat *= mat;

  combined_mat *= object_data->model_;

  if (object->debug()) {
	std::cout << "---------------------------------" << std::endl;
	std::cout << "Rendering object: " << object->name() << std::endl;
	std::cout << "Model: " << glm::to_string(object_data->model_) << std::endl;
	std::cout << "Combined model: " << glm::to_string(combined_mat) << std::endl;
	std::cout << "---------------------------------" << std::endl;
  }

  object_data->shader_->setMat4fv("projection", camera_->projectionMatrix());
  object_data->shader_->setMat4fv("view", camera_->viewMatrix());
  object_data->shader_->setMat4fv("model", combined_mat);

  if (object_data->material_) {
	object_data->shader_->setFloat("material.shininess", object_data->material_.value().shininess);
	object_data->shader_->setVec4("material.color", object_data->material_.value().color);
  }

  for (int no = 0; no < object_data->textures_.size(); no++)
	object_data->textures_.at(no)->activate(no);

  object_data->shader_->resetCounters();
  object_data->shader_->turnOffLights();
  if (lighting_) {
	auto lights = object->lights();

	for (auto light : lights) {
	  light->setup(object_data->shader_);
	}
  }

  object_data->shader_->use();

  glBindVertexArray(object_data->vao_);
  switch (object_data->render_type_) {
  case RenderType::Elements:
	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(object_data->count_),
				   GL_UNSIGNED_INT, 0);
	break;
  case RenderType::Array:glDrawArrays(GL_TRIANGLES, 0, object_data->count_);
	break;
  }
  glBindVertexArray(0);
}

auto OpenGLRender::render(std::shared_ptr<geometry::SkyBox> skybox) -> void {

  auto object_data = skybox->data();

  auto view = glm::mat4(glm::mat3(
	  camera_->viewMatrix()));  // remove translation from the view matrix

  object_data->shader_->setMat4fv("projection", camera_->projectionMatrix());
  object_data->shader_->setMat4fv("view", view);

  if (object_data->textures_.size())
	object_data->textures_.at(0)->activate(0);

  glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when
  // values are equal to depth buffer's content
  object_data->shader_->use();

  glBindVertexArray(object_data->vao_);
  glDrawArrays(GL_TRIANGLES, 0, object_data->count_);
  glDepthFunc(GL_LESS);  // set depth function back to default
}

auto OpenGLRender::render(const geometry::BoundingBox &box) -> void {
  auto center = box.center();
  auto max = box.max();
  auto min = box.min();

  std::cout << "Center => (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
  std::cout << "Size (WxHxD) => (" << max.x - min.x << ", " << max.y - min.y << ", " << max.z - min.z << ")"
			<< std::endl;

  if (!debug_shader_ || !debug_texture_) {
	std::cout << "No debug shader set f" << std::endl;
	return;
  }

  glm::mat4 combined_mat = glm::mat4(1.0f);

  for (auto mat : stack_)
	combined_mat *= mat;

//  combined_mat *= box.model();

  debug_shader_->setMat4fv("projection", camera_->projectionMatrix());
  debug_shader_->setMat4fv("view", camera_->viewMatrix());
  debug_shader_->setMat4fv("model", combined_mat);

  debug_shader_->setFloat("material.shininess", 0.5f);
  debug_shader_->setVec4("material.color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

  debug_texture_->activate(0);

  debug_shader_->resetCounters();
  debug_shader_->turnOffLights();
  debug_shader_->use();

  float vertices[] = {
	  // positions          // normals           // texture coords
	  -min.x, -min.y, -min.z, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
	  -min.x, max.y, -min.z, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
	  max.x, max.y, -min.z, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
	  max.x, max.y, -min.z, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
	  max.x, -min.y, -min.z, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
	  -min.x, -min.y, -min.z, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,

	  -min.x, -min.y, max.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	  max.x, -min.y, max.z, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
	  max.x, max.y, max.z, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	  max.x, max.y, max.z, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	  -min.x, max.y, max.z, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	  -min.x, -min.y, max.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

	  -min.x, max.y, max.z, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	  -min.x, max.y, -min.z, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	  -min.x, -min.y, -min.z, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	  -min.x, -min.y, -min.z, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	  -min.x, -min.y, max.z, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	  -min.x, max.y, max.z, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,

	  max.x, max.y, max.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Start
	  max.x, -min.y, max.z, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	  max.x, -min.y, -min.z, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	  max.x, -min.y, -min.z, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	  max.x, max.y, -min.z, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	  max.x, max.y, max.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	  -min.x, -min.y, -min.z, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
	  max.x, -min.y, -min.z, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
	  max.x, -min.y, max.z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	  max.x, -min.y, max.z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	  -min.x, -min.y, max.z, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	  -min.x, -min.y, -min.z, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

	  -min.x, max.y, -min.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	  -min.x, max.y, max.z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	  max.x, max.y, max.z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	  max.x, max.y, max.z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	  max.x, max.y, -min.z, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	  -min.x, max.y, -min.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};

  unsigned int VBO, cubeVAO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &VBO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindVertexArray(cubeVAO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float),
						(void *)(3*sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float),
						(void *)(6*sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(cubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &cubeVAO);
}

auto OpenGLRender::render(std::shared_ptr<interface::Light> object, std::shared_ptr<render::Shader> shader) -> void {

}
