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
#include <assimp/material.h>

#include <string>
#include <sstream>
#include <system/Log.h>

using namespace omega::utils;
using namespace omega::render;
using namespace omega::geometry;
using namespace std;

auto Loader::loadModel(std::string path) -> ObjectNodePtr {
  auto bytes = fs::instance()->data(path);
  auto ext = fs::instance()->extension(path);
  auto tree = std::make_shared<ObjectNode>();

  if (bytes.size()==0) {
	OMEGA_LOG_ERROR("loader", "Error loading file => {}", path);
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
	OMEGA_LOG_ERROR("loader", "ASSIMP error: {}", importer.GetErrorString());
	return nullptr;
  }

// process ASSIMP's root node recursively
  tree = processNode(scene->mRootNode, scene);

  return tree;
}

auto Loader::loadModelPreTransformed(std::string path) -> ObjectNodePtr {
  auto bytes = fs::instance()->data(path);
  auto ext = fs::instance()->extension(path);

  if (bytes.size() == 0) {
	OMEGA_LOG_ERROR("loader", "Error loading file => {}", path);
	return nullptr;
  }

  Assimp::Importer importer;
  // aiProcess_PreTransformVertices bakes each node's cumulative transform
  // into its vertex positions and promotes every mesh to the (single) root
  // node. Combined with Triangulate + normal/tangent generation this gives
  // us a flat, render-ready tree suitable for the NPC system.
  const aiScene *scene = importer.ReadFileFromMemory(
	  bytes.data(), bytes.size(),
	  aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		  aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices,
	  ext.c_str());

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	  !scene->mRootNode) {
	OMEGA_LOG_ERROR("loader", "ASSIMP error (pre-xform): {}",
					importer.GetErrorString());
	return nullptr;
  }

  return processNode(scene->mRootNode, scene);
}

auto Loader::processNode(aiNode *node, const aiScene *scene) -> ObjectNodePtr {
  auto tree = std::make_shared<ObjectNode>();

  // Convert the node's transform from Assimp (row-major: aN, bN, cN, dN are
  // row 1..4 of column N) to glm (column-major, constructor takes columns in
  // order). The naive per-element layout is:
  //   col 0 = (a1, b1, c1, d1)   col 1 = (a2, b2, c2, d2)
  //   col 2 = (a3, b3, c3, d3)   col 3 = (a4, b4, c4, d4)
  //
  // Historical note: the original open-coded constructor here had `c3`
  // spelled in place of `c1` on column 0, which produced a shear that baked
  // x into z for every loaded mesh. The NPC system sees that shear as the
  // silhouette appearing to warp whenever the animal rotates. Keep the
  // explicit column-wise form below so that bug cannot come back.
  const auto& t = node->mTransformation;
  glm::mat4x4 mat(t.a1, t.b1, t.c1, t.d1,   // column 0
                  t.a2, t.b2, t.c2, t.d2,   // column 1
                  t.a3, t.b3, t.c3, t.d3,   // column 2
                  t.a4, t.b4, t.c4, t.d4);  // column 3

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
  const bool hasDiffuseTexture = !diffuseMaps.empty();
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

  // Material colour — Blinn-Phong diffuse first, glTF PBR base-colour as a
  // fallback. Many of the assets we load through this path (notably Kenney's
  // animal GLBs) encode per-part tint as `baseColorFactor` with no diffuse
  // image, so without this read the mesh has nothing to sample and the
  // prop.fs / core.fs shaders collapse it to black. We keep alpha — the
  // Blinn-Phong shader path in Object::render treats alpha > 0.5 as "use
  // the explicit `material.color` tint" which is exactly what we want.
  aiColor4D matColor(1.0f, 1.0f, 1.0f, 1.0f);
  bool haveMaterialColor = false;
  if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &matColor) ==
	  AI_SUCCESS) {
	haveMaterialColor = true;
  } else if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &matColor) ==
			 AI_SUCCESS) {
	haveMaterialColor = true;
  }

  // return a mesh object created from the extracted mesh data
  auto object = utils::ObjectGenerator::mesh({.vertices = vertices,
												 .indices = indices,
												 .textures = textures,
												 .name = name});

  if (object && haveMaterialColor) {
	render::Material m;
	// Force alpha = 1.0 so the Object::render path routes through the
	// `material.color` branch of core.fs / prop.fs (both shaders use
	// `color.a > 0.5` as the selector between `color.rgb` and the
	// `diffuseColor` fallback).
	m.color = glm::vec4(matColor.r, matColor.g, matColor.b, 1.0f);
	m.diffuse = glm::vec3(matColor.r, matColor.g, matColor.b);
	m.opacity = matColor.a > 0.0f ? matColor.a : 1.0f;
	object->setMaterial(m);
  }

  // When the source material had no diffuse texture file, bind the cached
  // 1x1 white Texture at unit 0. Both core.fs and prop.fs sample
  // `material.diffuse` unconditionally; an unbound GL_TEXTURE0 returns
  // zeros on most drivers, which multiplies the tint to black. A white
  // fallback makes the colour tint the actual shade of the mesh and
  // leaves the textured path (trees, rocks) untouched.
  if (object && !hasDiffuseTexture) {
	const auto existing = object->getTextures();
	std::vector<std::shared_ptr<render::Texture>> withWhite;
	withWhite.reserve(existing.size() + 1);
	withWhite.push_back(render::Texture::defaultWhite());
	for (const auto& t : existing) withWhite.push_back(t);
	object->setTextures(withWhite);
  }

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