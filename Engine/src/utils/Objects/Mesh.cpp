#include <utils/ObjectGenerator.h>
#include <render/Shader.h>
#include <render/CubeTexture.h>
#include <geometry/SkyBox.h>
#include <geometry/Object.h>
#include <utils/utils.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace omega::utils;
using namespace omega::geometry;
using namespace omega::render;

auto ObjectGenerator::mesh(input::MeshInput input)
-> std::shared_ptr<omega::geometry::Object> {
  unsigned int VBO, EBO;
  unsigned int VAO;

  if (input.flags & ogMirrorUV)
	mirrorUV(input.vertices);

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  // load data into vertex buffers
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // A great thing about structs is that their memory layout is sequential for
  // all its items. The effect is that we can simply pass a pointer to the
  // struct and it translates perfectly to a glm::vec3/2 array which again
  // translates to 3/2 floats which translates to a byte array.
  glBufferData(GL_ARRAY_BUFFER, input.vertices.size()*sizeof(Vertex),
			   &input.vertices[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			   input.indices.size()*sizeof(unsigned int), &input.indices[0],
			   GL_STATIC_DRAW);

  // set the vertex attribute pointers
  // vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  // vertex normals
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						(void *)offsetof(Vertex, normal));
  // vertex texture coords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						(void *)offsetof(Vertex, uv));
  // vertex tangent
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						(void *)offsetof(Vertex, tangent));
  // vertex bitangent
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						(void *)offsetof(Vertex, bitangent));
  glBindVertexArray(0);

  auto object = std::make_shared<Object>(VAO, VBO, input.indices.size(),
										 RenderType::Elements);

  for (const auto &[key, value] : input.textures) {
	object->addTexture(value);
  }

  object->setName(input.name);

  auto list = split(input.name, {'_', ' ', '.'});
  for (auto item : list) {
	auto list2 = split(item, {':'});
	if (list2.size() > 1) {
	  if (list2[0]=="n")
		object->setName(list2[1]);
	  else if (list2[0]=="b") {
		physics::BodyType bodyType{physics::BodyType::STATIC};
		physics::ColliderType colliderType{physics::ColliderType::BOX};
		auto boundingBox = utils::bounding_box(input.vertices);

		if (list2[1]=="b")
		  colliderType = physics::ColliderType::BOX;
		else if (list2[1]=="s")
		  colliderType = physics::ColliderType::SPHERE;
		else if (list2[1]=="p")
		  colliderType = physics::ColliderType::PLANE;

		if (list2.size() > 2) {
		  if (list2[2]=="d")
			bodyType = physics::BodyType::DYNAMIC;
		  else if (list2[2]=="s")
			bodyType = physics::BodyType::STATIC;
		}

		object->physics({
							.bodyType = bodyType,
							.colliderType = colliderType,
							.boundingBox = boundingBox
						});
	  }
	}
  };

  return object;
}


