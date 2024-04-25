#include <utils/ObjectGenerator.h>
#include <render/Shader.h>
#include <render/CubeTexture.h>
#include <geometry/SkyBox.h>
#include <geometry/Object.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::utils;
using namespace omega::geometry;
using namespace omega::render;

auto ObjectGenerator::container(input::ObjectGenerator input)
-> std::shared_ptr<geometry::Object> {
  float containerSizeX =
	  2.5f*input.size;
  float containerSizeY = input.size;
  float containerSizeZ = input.size;


  // set up vertex data (and buffer(s)) and configure vertex attributes
  // ------------------------------------------------------------------
  float vertices[] = {
	  // positions          // normals           // texture coords
	  -containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.761f, 0.0f,  // Left
	  -containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.761f, 0.326f,
	  containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.0f, 0.326f,
	  containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.0f, 0.326f,
	  containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
	  -containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, 0.0f, -1.0f, 0.761f, 0.0f,

	  -containerSizeX, -containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // Side1
	  containerSizeX, -containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.761f, 0.0f,
	  containerSizeX, containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.761f, 0.326f,
	  containerSizeX, containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.761f, 0.326f,
	  -containerSizeX, containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.326f,
	  -containerSizeX, -containerSizeY, containerSizeZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

	  -containerSizeX, containerSizeY, containerSizeZ, -1.0f, 0.0f, 0.0f, 0.33f, 1.0f,  // End
	  -containerSizeX, containerSizeY, -containerSizeZ, -1.0f, 0.0f, 0.0f, 0.00f, 1.0f,
	  -containerSizeX, -containerSizeY, -containerSizeZ, -1.0f, 0.0f, 0.0f, 0.00f, 0.649f,
	  -containerSizeX, -containerSizeY, -containerSizeZ, -1.0f, 0.0f, 0.0f, 0.00f, 0.649f,
	  -containerSizeX, -containerSizeY, containerSizeZ, -1.0f, 0.0f, 0.0f, 0.33f, 0.649f,
	  -containerSizeX, containerSizeY, containerSizeZ, -1.0f, 0.0f, 0.0f, 0.33f, 1.0f,

	  containerSizeX, containerSizeY, containerSizeZ, 1.0f, 0.0f, 0.0f, 0.33f, 1.0f, // Start
	  containerSizeX, -containerSizeY, containerSizeZ, 1.0f, 0.0f, 0.0f, 0.33f, 0.649f,
	  containerSizeX, -containerSizeY, -containerSizeZ, 1.0f, 0.0f, 0.0f, 0.657f, 0.649f,
	  containerSizeX, -containerSizeY, -containerSizeZ, 1.0f, 0.0f, 0.0f, 0.657f, 0.649f,
	  containerSizeX, containerSizeY, -containerSizeZ, 1.0f, 0.0f, 0.0f, 0.657f, 1.0f,
	  containerSizeX, containerSizeY, containerSizeZ, 1.0f, 0.0f, 0.0f, 0.33f, 1.0f,

	  -containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, -1.0f, 0.0f, 0.0f, 0.326f, // Bottom
	  containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, -1.0f, 0.0f, 0.761f, 0.326f,
	  containerSizeX, -containerSizeY, containerSizeZ, 0.0f, -1.0f, 0.0f, 0.761f, 0.649f,
	  containerSizeX, -containerSizeY, containerSizeZ, 0.0f, -1.0f, 0.0f, 0.761f, 0.649f,
	  -containerSizeX, -containerSizeY, containerSizeZ, 0.0f, -1.0f, 0.0f, 0.0f, 0.649f,
	  -containerSizeX, -containerSizeY, -containerSizeZ, 0.0f, -1.0f, 0.0f, 0.0f, 0.326f,

	  -containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 1.0f, 0.0f, 0.0f, 0.326f, // Top
	  -containerSizeX, containerSizeY, containerSizeZ, 0.0f, 1.0f, 0.0f, 0.0f, 0.649f,
	  containerSizeX, containerSizeY, containerSizeZ, 0.0f, 1.0f, 0.0f, 0.761f, 0.649f,
	  containerSizeX, containerSizeY, containerSizeZ, 0.0f, 1.0f, 0.0f, 0.761f, 0.649f,
	  containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 1.0f, 0.0f, 0.761f, 0.326f,
	  -containerSizeX, containerSizeY, -containerSizeZ, 0.0f, 1.0f, 0.0f, 0.0f, 0.326f};

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

  auto object = std::make_shared<Object>(cubeVAO, VBO, 36);

  object->setTextures(input.textures);
  object->setShader(input.shader);
  object->setName(input.name);

  if (input.position)
	object->translate(input.position.value());
  if (input.matrix)
	object->setModel(input.matrix.value());
  if (input.material)
	object->setMaterial(input.material.value());
  if (input.lights)
	object->affectedByLights(input.lights.value());

  object->physics({
					  .bodyType = physics::BodyType::STATIC,
					  .colliderType = physics::ColliderType::BOX,
					  .boundingBox = {-containerSizeX, -containerSizeY, -containerSizeZ, containerSizeX, containerSizeY, containerSizeZ},
					  .mass = input.mass,
					  .bounciness = 0.0f,
				  });

  return object;
}
