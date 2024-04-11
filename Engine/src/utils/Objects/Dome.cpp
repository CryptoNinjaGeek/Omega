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

auto ObjectGenerator::dome(omega::input::CubeTextureInput input, float size)
-> std::shared_ptr<omega::geometry::Object> {
  float skyboxVertices[] = {
	  // positions
	  -size, size, -size, -size, -size, -size, size, -size, -size,
	  size, -size, -size, size, size, -size, -size, size, -size,

	  -size, -size, size, -size, -size, -size, -size, size, -size,
	  -size, size, -size, -size, size, size, -size, -size, size,

	  size, -size, -size, size, -size, size, size, size, size,
	  size, size, size, size, size, -size, size, -size, -size,

	  -size, -size, size, -size, size, size, size, size, size,
	  size, size, size, size, -size, size, -size, -size, size,

	  -size, size, -size, size, size, -size, size, size, size,
	  size, size, size, -size, size, size, -size, size, -size,

	  -size, -size, -size, -size, -size, size, size, -size, -size,
	  size, -size, -size, -size, -size, size, size, -size, size};

  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
			   GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void *)0);

  auto cubeTexture = std::make_shared<CubeTexture>();
  cubeTexture->load(input);

  auto object = std::make_shared<SkyBox>(skyboxVAO, skyboxVBO, 36);
  object->addTexture(cubeTexture);

  return object;
}


