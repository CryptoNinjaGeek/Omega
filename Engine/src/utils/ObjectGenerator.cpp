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

auto ObjectGenerator::box(input::ObjectGenerator input)
    -> std::shared_ptr<geometry::Object> {
  // set up vertex data (and buffer(s)) and configure vertex attributes
  // ------------------------------------------------------------------
  float vertices[] = {
      // positions          // normals           // texture coords
      -input.size, -input.size, -input.size, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f,
      input.size,  -input.size, -input.size, 0.0f,  0.0f,  -1.0f, 1.0f, 0.0f,
      input.size,  input.size,  -input.size, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f,
      input.size,  input.size,  -input.size, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f,
      -input.size, input.size,  -input.size, 0.0f,  0.0f,  -1.0f, 0.0f, 1.0f,
      -input.size, -input.size, -input.size, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f,

      -input.size, -input.size, input.size,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
      input.size,  -input.size, input.size,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
      input.size,  input.size,  input.size,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
      input.size,  input.size,  input.size,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
      -input.size, input.size,  input.size,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
      -input.size, -input.size, input.size,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

      -input.size, input.size,  input.size,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f,
      -input.size, input.size,  -input.size, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f,
      -input.size, -input.size, -input.size, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
      -input.size, -input.size, -input.size, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
      -input.size, -input.size, input.size,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f,
      -input.size, input.size,  input.size,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f,

      input.size,  input.size,  input.size,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
      input.size,  input.size,  -input.size, 1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
      input.size,  -input.size, -input.size, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
      input.size,  -input.size, -input.size, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
      input.size,  -input.size, input.size,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
      input.size,  input.size,  input.size,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

      -input.size, -input.size, -input.size, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f,
      input.size,  -input.size, -input.size, 0.0f,  -1.0f, 0.0f,  1.0f, 1.0f,
      input.size,  -input.size, input.size,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f,
      input.size,  -input.size, input.size,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f,
      -input.size, -input.size, input.size,  0.0f,  -1.0f, 0.0f,  0.0f, 0.0f,
      -input.size, -input.size, -input.size, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f,

      -input.size, input.size,  -input.size, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
      input.size,  input.size,  -input.size, 0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
      input.size,  input.size,  input.size,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
      input.size,  input.size,  input.size,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
      -input.size, input.size,  input.size,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
      -input.size, input.size,  -input.size, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f};

  unsigned int VBO, cubeVAO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &VBO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindVertexArray(cubeVAO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  auto object = std::make_shared<Object>(cubeVAO, VBO, 36);

  object->setTextures(input.textures);
  object->setShader(input.shader);

  if (input.position) object->setPosition(input.position.value());
  if (input.matrix) object->setModel(input.matrix.value());
  if (input.material) object->setMaterial(input.material.value());
  if (input.lights) object->affectedByLights(input.lights.value());

  return object;
}

auto ObjectGenerator::dome(omega::input::CubeTextureInput input, float size)
    -> std::shared_ptr<omega::geometry::Object> {
  float skyboxVertices[] = {
      // positions
      -size, size,  -size, -size, -size, -size, size,  -size, -size,
      size,  -size, -size, size,  size,  -size, -size, size,  -size,

      -size, -size, size,  -size, -size, -size, -size, size,  -size,
      -size, size,  -size, -size, size,  size,  -size, -size, size,

      size,  -size, -size, size,  -size, size,  size,  size,  size,
      size,  size,  size,  size,  size,  -size, size,  -size, -size,

      -size, -size, size,  -size, size,  size,  size,  size,  size,
      size,  size,  size,  size,  -size, size,  -size, -size, size,

      -size, size,  -size, size,  size,  -size, size,  size,  size,
      size,  size,  size,  -size, size,  size,  -size, size,  -size,

      -size, -size, -size, -size, -size, size,  size,  -size, -size,
      size,  -size, -size, -size, -size, size,  size,  -size, size};

  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

  auto cubeTexture = std::make_shared<CubeTexture>();
  cubeTexture->load(input);

  auto object = std::make_shared<SkyBox>(skyboxVAO, skyboxVBO, 36);
  object->addTexture(cubeTexture);

  return object;
}

auto ObjectGenerator::plane(input::ObjectGenerator input)
    -> std::shared_ptr<omega::geometry::Object> {
  // set up vertex data (and buffer(s)) and configure vertex attributes
  // ------------------------------------------------------------------
  float vertices[] = {
      // positions          // normals           // texture coords
      -input.size, 0.f, -input.size, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      input.size,  0.f, -input.size, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      input.size,  0.f, input.size,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      input.size,  0.f, input.size,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      -input.size, 0.f, input.size,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      -input.size, 0.f, -input.size, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};

  unsigned int VBO, cubeVAO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &VBO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindVertexArray(cubeVAO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  auto object = std::make_shared<Object>(cubeVAO, VBO, 6);

  object->setTextures(input.textures);
  object->setShader(input.shader);

  if (input.position) object->setPosition(input.position.value());
  if (input.matrix) object->setModel(input.matrix.value());
  if (input.material) object->setMaterial(input.material.value());
  if (input.lights) object->affectedByLights(input.lights.value());

  return object;
}
