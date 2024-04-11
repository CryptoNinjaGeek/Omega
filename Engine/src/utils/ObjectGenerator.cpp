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

auto ObjectGenerator::mirrorUV(std::vector<omega::geometry::Vertex> &vertices)
-> void {
  float min = 999.f, max = -999.f, middle;
  for (int nr = 0; nr < vertices.size(); nr++) {
	if (vertices[nr].uv.x > max)
	  max = vertices[nr].uv.x;
	if (vertices[nr].uv.x < min)
	  min = vertices[nr].uv.x;
  }
  middle = (max - min)/2.f;

  for (int nr = 0; nr < vertices.size(); nr++) {
	vertices[nr].uv.x = ((vertices[nr].uv.x - middle)*-1.f) + middle;
  }
}

