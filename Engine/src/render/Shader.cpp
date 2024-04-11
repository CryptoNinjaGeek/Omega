#include <iostream>
#include <fstream>
#include <string>

#include <render/Shader.h>
#include <system/FileSystem.h>

#if defined(WIN32)
#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL\gl.h>
#include <GL\glu.h>
#else
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace omega::render;
using namespace omega::geometry;

#define MAX_POINT 4
#define MAX_SPOT 4
#define MAX_DIRECTIONAL 4

std::string Shader::loadShaderSource(std::string fileName) {
  std::string temp = "";
  std::string src = "";
  std::cout << src << std::endl;

  std::ifstream in_file;

  // Vertex
  in_file.open(fileName);

  if (in_file.is_open()) {
	while (std::getline(in_file, temp))
	  src += temp + "\n";
  } else {
	std::cout << "ERROR::SHADER::COULD_NOT_OPEN_FILE: " << fileName << "\n";
  }

  in_file.close();
  return src;
}

GLuint Shader::loadShaderFromFile(GLenum type, std::string fileName) {
  char infoLog[512];
  GLint success;

  GLuint shader = glCreateShader(type);
  std::string str_src = fs::instance()->string(fileName);
  const GLchar *src = str_src.c_str();
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
	glGetShaderInfoLog(shader, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::COULD_NOT_COMPILE_SHADER: " << fileName
			  << "\n";
	std::cout << infoLog << "\n";
  }

  return shader;
}

GLuint Shader::loadShaderFromString(GLenum type, std::string str_src) {
  char infoLog[512];
  GLint success;

  GLuint shader = glCreateShader(type);
  const GLchar *src = str_src.c_str();
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
	glGetShaderInfoLog(shader, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::COULD_NOT_COMPILE_SHADER: " << str_src << "\n";
	std::cout << infoLog << "\n";
  }

  return shader;
}

void Shader::linkProgram(GLuint vertexShader, GLuint geometryShader,
						 GLuint fragmentShader) {
  char infoLog[512];
  GLint success;

  this->id = glCreateProgram();

  glAttachShader(this->id, vertexShader);

  if (geometryShader)
	glAttachShader(this->id, geometryShader);

  glAttachShader(this->id, fragmentShader);

  glLinkProgram(this->id);

  glGetProgramiv(this->id, GL_LINK_STATUS, &success);
  if (!success) {
	glGetProgramInfoLog(this->id, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::COULD_NOT_LINK_PROGRAM"
			  << "\n";
	std::cout << infoLog << "\n";
  }

  glUseProgram(0);
}

// Constructors/Destructors
Shader::Shader(const int versionMajor, const int versionMinor,
			   std::string vertexFile, std::string fragmentFile,
			   std::string geometryFile)
	: versionMajor(versionMajor), versionMinor(versionMinor) {
  GLuint vertexShader = 0;
  GLuint geometryShader = 0;
  GLuint fragmentShader = 0;

  vertexShader = loadShaderFromFile(GL_VERTEX_SHADER, vertexFile);

  if (!geometryFile.empty())
	geometryShader = loadShaderFromFile(GL_GEOMETRY_SHADER, geometryFile);

  fragmentShader = loadShaderFromFile(GL_FRAGMENT_SHADER, fragmentFile);

  this->linkProgram(vertexShader, geometryShader, fragmentShader);

  // End
  glDeleteShader(vertexShader);
  glDeleteShader(geometryShader);
  glDeleteShader(fragmentShader);
}

Shader::Shader(const int versionMajor, const int versionMinor)
	: versionMajor(versionMajor), versionMinor(versionMinor) {
  GLuint vertexShader = 0;
  GLuint geometryShader = 0;
  GLuint fragmentShader = 0;
}

Shader::~Shader() { glDeleteProgram(this->id); }

bool Shader::loadShadersFromString(std::string vertexCode,
								   std::string fragmentCode,
								   std::string geometryCode) {
  GLuint vertexShader = 0;
  GLuint geometryShader = 0;
  GLuint fragmentShader = 0;

  vertexShader = loadShaderFromString(GL_VERTEX_SHADER, vertexCode);

  if (!geometryCode.empty())
	geometryShader = loadShaderFromString(GL_GEOMETRY_SHADER, geometryCode);

  fragmentShader = loadShaderFromString(GL_FRAGMENT_SHADER, fragmentCode);

  this->linkProgram(vertexShader, geometryShader, fragmentShader);

  // End
  glDeleteShader(vertexShader);
  glDeleteShader(geometryShader);
  glDeleteShader(fragmentShader);
  return true;
}

bool Shader::loadShadersFromFile(std::string vertexFile,
								 std::string fragmentFile,
								 std::string geometryFile) {
  GLuint vertexShader = 0;
  GLuint geometryShader = 0;
  GLuint fragmentShader = 0;

  vertexShader = loadShaderFromFile(GL_VERTEX_SHADER, vertexFile);

  if (!geometryFile.empty())
	geometryShader = loadShaderFromFile(GL_GEOMETRY_SHADER, geometryFile);

  fragmentShader = loadShaderFromFile(GL_FRAGMENT_SHADER, fragmentFile);

  this->linkProgram(vertexShader, geometryShader, fragmentShader);

  // End
  glDeleteShader(vertexShader);
  glDeleteShader(geometryShader);
  glDeleteShader(fragmentShader);
  return true;
}

// Set uniform functions
void Shader::use() { glUseProgram(this->id); }

void Shader::unuse() { glUseProgram(0); }

void Shader::setInt(std::string name, GLint value) {
  this->use();

  glUniform1i(glGetUniformLocation(this->id, name.c_str()), value);

  this->unuse();
}

void Shader::setFloat(std::string name, GLfloat value) {
  this->use();

  glUniform1f(glGetUniformLocation(this->id, name.c_str()), value);

  this->unuse();
}

void Shader::setVec2(std::string name, glm::vec2 value) {
  this->use();

  glUniform2fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);

  this->unuse();
}

void Shader::setVec3(std::string name, glm::vec3 value) {
  this->use();

  glUniform3fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);

  this->unuse();
}

void Shader::setVec4(std::string name, glm::vec4 value) {
  this->use();

  glUniform4fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);

  this->unuse();
}

void Shader::setMat4fv(const std::string name, glm::mat4 value,
					   bool transpose) {
  this->use();

  glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, GL_FALSE,
					 &value[0][0]);

  this->unuse();
}

void Shader::setVec3(std::string name, float x, float y, float z) {
  setVec3(name, glm::vec3(x, y, z));
}

void Shader::setVec4(std::string name, float x, float y, float z, float w) {
  setVec4(name, glm::vec4(x, y, z, w));
}

std::shared_ptr<Shader> Shader::fromString(const int versionMajor,
										   const int versionMinor,
										   std::string vertexCode,
										   std::string fragmentCode,
										   std::string geometryCode) {
  auto ptr = std::make_shared<Shader>(versionMajor, versionMinor);

  ptr->loadShadersFromString(vertexCode, fragmentCode, geometryCode);

  return ptr;
}

std::shared_ptr<Shader> Shader::fromFile(const int versionMajor,
										 const int versionMinor,
										 std::string vertexFile,
										 std::string fragmentFile,
										 std::string geometryFile) {
  auto ptr = std::make_shared<Shader>(versionMajor, versionMinor);

  ptr->loadShadersFromFile(vertexFile, fragmentFile, geometryFile);

  return ptr;
}

auto Shader::getLightNumber(interface::LightType type) -> int {
  switch (type) {
  case interface::LightType::POINT:
	if (point_lights_++ > MAX_POINT)
	  return -1;
	return point_lights_ - 1;
  case interface::LightType::SPOT:
	if (spot_lights_++ > MAX_SPOT)
	  return -1;
	return spot_lights_ - 1;
  case interface::LightType::DIRECTIONAL:
	if (directional_lights_++ > MAX_DIRECTIONAL)
	  return -1;
	return directional_lights_ - 1;
  }
  return -1;
}

auto Shader::turnOffLights() -> void {
  for (int no = 0; no < MAX_SPOT; no++)
	setInt("spotLight[" + std::to_string(no) + "].on", 0);
  for (int no = 0; no < MAX_POINT; no++)
	setInt("pointLights[" + std::to_string(no) + "].on", 0);
  for (int no = 0; no < MAX_DIRECTIONAL; no++)
	setInt("dirLight[" + std::to_string(no) + "].on", 0);
}
