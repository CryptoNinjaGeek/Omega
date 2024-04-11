#include <utils/Loader.h>
#include <system/FileSystem.h>
#include <geometry/Object.h>
#include <utils/ObjectGenerator.h>
#include <render/Texture.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/texture.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace omega::utils;
using namespace omega::render;
using namespace omega::geometry;
using namespace std;

auto Loader::loadModel(std::string path) -> ObjectNodePtr {
  auto bytes = fs::instance()->data(path);
  auto ext = fs::instance()->extension(path);
  auto tree = std::make_shared<ObjectNode>();

  if (bytes.size()==0) {
	std::cout << "Error loading file => " << path << std::endl;
	return nullptr;
  }

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFileFromMemory(
	  bytes.data(), bytes.size(),
	  aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		  aiProcess_CalcTangentSpace, ext.c_str());
// check for errors
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	  !scene->mRootNode)  // if is Not Zero
  {
	cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
	return nullptr;
  }

// process ASSIMP's root node recursively
  tree = processNode(scene->mRootNode, scene);

  return tree;
}

auto Loader::processNode(aiNode *node, const aiScene *scene) -> ObjectNodePtr {
  auto tree = std::make_shared<ObjectNode>();

  // process each mesh located at the current node
  glm::mat4x4 mat(node->mTransformation.a1, node->mTransformation.b1,
				  node->mTransformation.c3, node->mTransformation.d1,
				  node->mTransformation.a2, node->mTransformation.b2,
				  node->mTransformation.c2, node->mTransformation.d2,
				  node->mTransformation.a3, node->mTransformation.b3,
				  node->mTransformation.c3, node->mTransformation.d3,
				  node->mTransformation.a4, node->mTransformation.b4,
				  node->mTransformation.c4, node->mTransformation.d4);

  tree->mat = mat;

  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
	// the node object only contains indices to index the actual objects in
	// the scene. the scene contains all the data, node is just to keep
	// stuff organized (like relations between nodes).
	aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
	auto object = processMesh(node->mName.C_Str(), mesh, scene);
	if (object)
	  tree->meshes.push_back(object);
  }
  // after we've processed all of the meshes (if any) we then recursively
  // process each of the children nodes
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
	auto child = processNode(node->mChildren[i], scene);
	tree->children.push_back(child);
  }
  return tree;
}

shared_ptr<Object> Loader::processMesh(std::string name, aiMesh *mesh,
									   const aiScene *scene) {

  float minx, miny, minz, maxx, maxy, maxz;
  // data to fill
  vector<Vertex> vertices;
  vector<unsigned int> indices;
  map<string, shared_ptr<Texture>> textures;

  // walk through each of the mesh's vertices
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
	Vertex vertex;
	glm::vec3 vector;  // we declare a placeholder vector since assimp uses
	// its own vector class that doesn't directly convert
	// to glm's vec3 class so we transfer the data to
	// this placeholder glm::vec3 first.
	// positions
	vector.x = mesh->mVertices[i].x;
	vector.y = mesh->mVertices[i].y;
	vector.z = mesh->mVertices[i].z;
	vertex.position = vector;

	if (i==0) {
	  minx = maxx = vector.x;
	  miny = maxy = vector.y;
	  minz = maxz = vector.z;
	} else {
	  minx = min(minx, vector.x);
	  miny = min(miny, vector.y);
	  minz = min(minz, vector.z);
	  maxx = max(maxx, vector.x);
	  maxy = max(maxy, vector.y);
	  maxz = max(maxz, vector.z);
	}

	// normals
	if (mesh->HasNormals()) {
	  vector.x = mesh->mNormals[i].x;
	  vector.y = mesh->mNormals[i].y;
	  vector.z = mesh->mNormals[i].z;
	  vertex.normal = vector;
	}
	// texture coordinates
	if (mesh->mTextureCoords[0])  // does the mesh contain texture
	  // coordinates?
	{
	  glm::vec2 vec;
	  // a vertex can contain up to 8 different texture coordinates. We thus
	  // make the assumption that we won't use models where a vertex can
	  // have multiple texture coordinates so we always take the first set
	  // (0).
	  vec.x = mesh->mTextureCoords[0][i].x;
	  vec.y = mesh->mTextureCoords[0][i].y;
	  vertex.uv = vec;
	  // tangent
	  vector.x = mesh->mTangents[i].x;
	  vector.y = mesh->mTangents[i].y;
	  vector.z = mesh->mTangents[i].z;
	  vertex.tangent = vector;
	  // bitangent
	  vector.x = mesh->mBitangents[i].x;
	  vector.y = mesh->mBitangents[i].y;
	  vector.z = mesh->mBitangents[i].z;
	  vertex.bitangent = vector;
	} else
	  vertex.uv = glm::vec2(0.0f, 0.0f);

	vertices.push_back(vertex);
  }
  // now wak through each of the mesh's faces (a face is a mesh its
  // triangle) and retrieve the corresponding vertex indices.
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
	aiFace face = mesh->mFaces[i];
	// retrieve all indices of the face and store them in the indices vector
	for (unsigned int j = 0; j < face.mNumIndices; j++)
	  indices.push_back(face.mIndices[j]);
  }
  // process materials
  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  // we assume a convention for sampler names in the shaders. Each diffuse
  // texture should be named as 'texture_diffuseN' where N is a sequential
  // number ranging from 1 to MAX_SAMPLER_NUMBER. Same applies to other
  // texture as the following list summarizes: diffuse: texture_diffuseN
  // specular: texture_specularN
  // normal: texture_normalN

  // 1. diffuse maps
  auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE,
										  "texture_diffuse");
  textures.merge(diffuseMaps);
  // 2. specular maps
  auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR,
										   "texture_specular");
  textures.merge(specularMaps);
  // 3. normal maps
  auto normalMaps =
	  loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
  textures.merge(normalMaps);
  // 4. height maps
  auto heightMaps =
	  loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
  textures.merge(heightMaps);

  // return a mesh object created from the extracted mesh data
  auto object = utils::ObjectGenerator::mesh({.vertices = vertices,
												 .indices = indices,
												 .textures = textures,
												 .name = name});

  return object;
}

std::map<std::string, std::shared_ptr<Texture>> Loader::loadMaterialTextures(aiMaterial *mat,
																			 aiTextureType type,
																			 std::string typeName) {
  std::map<std::string, std::shared_ptr<Texture>> textures;

  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
	std::shared_ptr<Texture> texture = std::make_shared<Texture>();
	aiString str;
	mat->GetTexture(type, i, &str);

	if (!texture->load(str.C_Str()))
	  continue;

	textures[str.C_Str()] = texture;
  }
  return textures;
}