#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <system/Global.h>
#include <geometry/Matrix.h>
#include <geometry/Point2.h>
#include <geometry/Point3.h>
#include <geometry/Point4.h>

#include <interface/Light.h>

#include "glm/mat4x4.hpp"

namespace omega {
namespace render {
using namespace omega::geometry;

class OMEGA_EXPORT Shader {
private:
  unsigned int id;
  const int versionMajor;
  const int versionMinor;

  // Private functions
  std::string loadShaderSource(std::string fileName);
  unsigned int loadShaderFromFile(unsigned int type, std::string fileName);
  unsigned int loadShaderFromString(unsigned int type, std::string fileName);
  bool loadShadersFromString(std::string vertexCode, std::string fragmentCode,
							 std::string geometryCode = {});
  bool loadShadersFromFile(std::string vertexFile, std::string fragmentFile,
						   std::string geometryFile = {});

  void linkProgram(unsigned int vertexShader, unsigned int geometryShader,
				   unsigned int fragmentShader);

public:
  Shader(const int versionMajor, const int versionMinor);

  Shader(const int versionMajor, const int versionMinor, std::string vertexFile,
		 std::string fragmentFile, std::string geometryFile = {});

  ~Shader();

  // Set uniform functions
  void use();
  void unuse();
  void setInt(std::string name, int value);
  void setFloat(std::string name, float value);
  void setVec2(std::string name, glm::vec2 value);
  void setVec3(std::string name, glm::vec3 value);
  void setVec4(std::string name, glm::vec4 value);
  void setMat4fv(std::string name, glm::mat4 value, bool transpose = false);

  void setVec4(std::string name, float x, float y, float z, float w);
  void setVec3(std::string name, float x, float y, float z);

  void resetCounters() {
	point_lights_ = 0;
	spot_lights_ = 0;
	directional_lights_ = 0;
  }

  auto getLightNumber(interface::LightType) -> int;
  auto turnOffLights() -> void;

  static std::shared_ptr<Shader> fromString(const int versionMajor,
											const int versionMinor,
											std::string vertexCode,
											std::string fragmentCode,
											std::string geometryCode = {});

  static std::shared_ptr<Shader> fromFile(const int versionMajor,
										  const int versionMinor,
										  std::string vertexFile,
										  std::string fragmentFile,
										  std::string geometryFile = {});

private:
  unsigned int point_lights_{0};
  unsigned int spot_lights_{0};
  unsigned int directional_lights_{0};
};

}  // namespace render
}  // namespace omega
