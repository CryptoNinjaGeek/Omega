#include <geometry/PortalSurface.h>
#include <geometry/Portal.h>
#include <geometry/Object.h>
#include <geometry/Vertex.h>
#include <render/Shader.h>
#include <glad/glad.h>

using namespace omega::geometry;
using namespace omega::render;

std::shared_ptr<Object> PortalSurface::createSurface(
    std::shared_ptr<Portal> portal,
    std::shared_ptr<Shader> shader) {
  if (!portal) {
    return nullptr;
  }

  // Generate vertices and indices for portal quad
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  
  generatePortalVertices(*portal, vertices, indices);

  if (vertices.empty() || indices.empty()) {
    return nullptr;
  }

  // Create VAO and VBO
  unsigned int VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  // Upload vertex data
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  // Upload index data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  // Set vertex attributes
  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                       (void*)offsetof(Vertex, position));
  
  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                       (void*)offsetof(Vertex, normal));
  
  // Texture coordinates
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                       (void*)offsetof(Vertex, uv));

  glBindVertexArray(0);

  // Create Object with EBO
  // Note: Object constructor takes (VAO, VBO, count, type)
  // For Elements type, count is the number of indices
  auto object = std::make_shared<Object>(VAO, VBO, static_cast<unsigned int>(indices.size()),
                                         ObjectType::Elements);
  object->setShader(shader);
  object->setName("PortalSurface");

  return object;
}

void PortalSurface::generatePortalVertices(
    const Portal& portal,
    std::vector<geometry::Vertex>& vertices,
    std::vector<unsigned int>& indices) {
  vertices.clear();
  indices.clear();

  // Get portal corners
  glm::vec3 corners[4];
  portal.getCorners(corners);

  // Get portal normal and up vectors
  glm::vec3 normal = portal.getNormal();
  glm::vec3 up = portal.getUp();
  glm::vec3 right = portal.getRight();

  // Create vertices for quad
  // Top-left, top-right, bottom-right, bottom-left
  Vertex v0, v1, v2, v3;

  // Top-left
  v0.position = corners[0];
  v0.normal = normal;
  v0.uv = glm::vec2(0.0f, 1.0f);
  v0.tangent = glm::vec3(0.0f);
  v0.bitangent = glm::vec3(0.0f);

  // Top-right
  v1.position = corners[1];
  v1.normal = normal;
  v1.uv = glm::vec2(1.0f, 1.0f);
  v1.tangent = glm::vec3(0.0f);
  v1.bitangent = glm::vec3(0.0f);

  // Bottom-right
  v2.position = corners[2];
  v2.normal = normal;
  v2.uv = glm::vec2(1.0f, 0.0f);
  v2.tangent = glm::vec3(0.0f);
  v2.bitangent = glm::vec3(0.0f);

  // Bottom-left
  v3.position = corners[3];
  v3.normal = normal;
  v3.uv = glm::vec2(0.0f, 0.0f);
  v3.tangent = glm::vec3(0.0f);
  v3.bitangent = glm::vec3(0.0f);

  vertices.push_back(v0);
  vertices.push_back(v1);
  vertices.push_back(v2);
  vertices.push_back(v3);

  // Create indices for two triangles
  // Triangle 1: 0, 1, 2
  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);

  // Triangle 2: 0, 2, 3
  indices.push_back(0);
  indices.push_back(2);
  indices.push_back(3);
}

