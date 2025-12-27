#include <utils/PortalSceneLoader.h>
#include <system/FileSystem.h>
#include <geometry/Scene.h>
#include <geometry/Portal.h>
#include <geometry/PortalPair.h>
#include <geometry/PortalRenderer.h>
#include <render/CameraFPS.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <render/PortalFramebuffer.h>
#include <render/DirectionalLight.h>
#include <render/PointLight.h>
#include <render/SpotLight.h>
#include <render/Material.h>
#include <utils/ObjectGenerator.h>
#include <utils/Loader.h>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace omega::utils;
using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::input;

PortalSceneLoader::PortalSceneLoader() = default;

std::shared_ptr<Scene> PortalSceneLoader::loadFromFile(const std::string& filePath) {
  auto jsonString = fs::instance()->string(filePath);
  if (jsonString.empty()) {
    std::cerr << "Failed to load JSON file: " << filePath << std::endl;
    return nullptr;
  }
  return loadFromString(jsonString);
}

std::shared_ptr<Scene> PortalSceneLoader::loadFromString(const std::string& jsonString) {
  try {
    auto json = nlohmann::json::parse(jsonString);
    
    // Clear previous state
    portalPairs_.clear();
    portals_.clear();
    textures_.clear();
    materials_.clear();
    
    // Check version
    if (json.contains("version")) {
      std::string version = json["version"];
      if (version != "1.0") {
        std::cerr << "Warning: JSON version " << version << " may not be fully supported" << std::endl;
      }
    }
    
    // Parse scene
    if (json.contains("scene")) {
      parseScene(json["scene"]);
    } else {
      std::cerr << "Error: JSON missing 'scene' object" << std::endl;
      return nullptr;
    }
    
    // Create scene
    auto scene = std::make_shared<Scene>(false);
    
    // Parse textures first (needed by objects)
    if (json["scene"].contains("textures")) {
      parseTextures(json["scene"]["textures"]);
    }
    
    // Parse materials
    if (json["scene"].contains("materials")) {
      parseMaterials(json["scene"]["materials"]);
    }
    
    // Parse camera
    if (json["scene"].contains("camera")) {
      parseCamera(json["scene"]["camera"]);
    }
    
    // Load shaders
    auto shader = Shader::fromFile(4, 2, ":/shaders/core.vs", "./core.fs");
    shader->setInt("texture1", 0);
    
    auto plainShader = Shader::fromFile(4, 2, ":/shaders/plain.vs", ":/shaders/plain.fs");
    plainShader->setInt("texture1", 0);
    
    scene->shaders(shader, plainShader);
    
    // Set ambient
    if (json["scene"].contains("ambient")) {
      auto ambient = parseVec4(json["scene"], "ambient", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
      shader->setVec4("ambient", ambient.x, ambient.y, ambient.z, ambient.w);
    }
    
    // Parse objects
    if (json["scene"].contains("objects")) {
      parseObjects(json["scene"]["objects"], scene.get(), shader);
    }
    
    // Parse lights
    if (json["scene"].contains("lights")) {
      parseLights(json["scene"]["lights"], scene.get());
    }
    
    // Parse portals
    if (json["scene"].contains("portals")) {
      parsePortals(json["scene"]["portals"]);
    }
    
    // Create and configure portal renderer if portals exist
    if (!portalPairs_.empty()) {
      auto portalRenderer = std::make_shared<PortalRenderer>();
      for (auto& portalPair : portalPairs_) {
        portalRenderer->addPortalPair(portalPair);
      }
      portalRenderer->setMaxRecursionDepth(2);  // Default recursion depth
      portalRenderer->setEnabled(true);
      scene->setPortalRenderer(portalRenderer);
    }
    
    // Prepare scene (sets up physics)
    scene->prepare();
    
    return scene;
  } catch (const nlohmann::json::exception& e) {
    std::cerr << "JSON parsing error: " << e.what() << std::endl;
    return nullptr;
  }
}

void PortalSceneLoader::parseScene(const nlohmann::json& json) {
  // Scene name is optional
  if (json.contains("name")) {
    // Could store scene name if Scene class supports it
  }
}

void PortalSceneLoader::parseObjects(const nlohmann::json& json, Scene* scene,
                                     std::shared_ptr<Shader> defaultShader) {
  if (!json.is_array()) {
    return;
  }
  
  for (const auto& objJson : json) {
    if (!objJson.is_object()) continue;
    
    std::string type = parseString(objJson, "type", "box");
    std::string name = parseString(objJson, "name", "");
    auto position = parseVec3(objJson, "position");
    
    // Parse scale: can be a single float or vec3
    glm::vec3 scaleVec(1.0f);
    if (objJson.contains("scale")) {
      if (objJson["scale"].is_number()) {
        // Single float - uniform scale
        float scaleFloat = objJson["scale"].get<float>();
        scaleVec = glm::vec3(scaleFloat);
      } else if (objJson["scale"].is_array() && objJson["scale"].size() >= 3) {
        // Vec3 - non-uniform scale
        scaleVec = parseVec3(objJson, "scale", glm::vec3(1.0f));
      }
    }
    float scale = scaleVec.x; // Keep for backward compatibility
    
    // Parse rotation: Euler angles in degrees (vec3)
    glm::vec3 rotation = parseVec3(objJson, "rotation", glm::vec3(0.0f));
    
    float size = parseFloat(objJson, "size", 0.5f);
    float mass = parseFloat(objJson, "mass", 1.0f);
    bool visible = parseBool(objJson, "visible", true);
    
    // Get textures
    std::vector<std::shared_ptr<Texture>> objectTextures;
    if (objJson.contains("textures") && objJson["textures"].is_array()) {
      for (const auto& texName : objJson["textures"]) {
        if (texName.is_string()) {
          std::string texNameStr = texName;
          if (textures_.find(texNameStr) != textures_.end()) {
            objectTextures.push_back(textures_[texNameStr]);
          }
        }
      }
    }
    
    // Get material
    std::optional<Material> material;
    std::string materialName = parseString(objJson, "material", "");
    if (!materialName.empty() && materials_.find(materialName) != materials_.end()) {
      material = materials_[materialName];
    }
    
    // Get shader
    std::shared_ptr<Shader> shader = defaultShader;
    std::string shaderName = parseString(objJson, "shader", "core");
    // Shader selection would be handled by scene
    
    std::shared_ptr<Object> object;
    
    // Create object based on type
    if (type == "box") {
      input::ObjectGenerator input;
      input.position = position;
      input.shader = shader;
      input.textures = objectTextures;
      input.material = material;
      input.size = size;
      input.mass = mass;
      input.name = name;
      object = ObjectGenerator::box(input);
    } else if (type == "plane") {
      input::ObjectGenerator input;
      input.position = position;
      input.shader = shader;
      input.textures = objectTextures;
      input.material = material;
      input.size = size;
      input.mass = mass;
      input.name = name;
      object = ObjectGenerator::plane(input);
    } else if (type == "container") {
      input::ObjectGenerator input;
      input.position = position;
      input.shader = shader;
      input.textures = objectTextures;
      input.material = material;
      input.size = size;
      input.mass = mass;
      input.name = name;
      object = ObjectGenerator::container(input);
    } else if (type == "mesh") {
      // Check if it's a file-based mesh or custom geometry
      std::string meshFile = parseString(objJson, "meshFile", "");
      if (!meshFile.empty()) {
        // File-based mesh
        auto tree = Loader::loadModel(meshFile);
        if (tree) {
          scene->add(tree);
          continue;  // Mesh loaded as tree, skip object creation
        }
      }
      
      // Custom geometry mesh (vertices and indices defined in JSON)
      if (objJson.contains("vertices") && objJson.contains("indices")) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        // Parse vertices
        if (objJson["vertices"].is_array()) {
          for (const auto& vJson : objJson["vertices"]) {
            Vertex v;
            if (vJson.contains("position") && vJson["position"].is_array() && vJson["position"].size() >= 3) {
              v.position = glm::vec3(
                vJson["position"][0].get<float>(),
                vJson["position"][1].get<float>(),
                vJson["position"][2].get<float>()
              );
            }
            if (vJson.contains("normal") && vJson["normal"].is_array() && vJson["normal"].size() >= 3) {
              v.normal = glm::vec3(
                vJson["normal"][0].get<float>(),
                vJson["normal"][1].get<float>(),
                vJson["normal"][2].get<float>()
              );
            } else {
              v.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Default normal
            }
            if (vJson.contains("uv") && vJson["uv"].is_array() && vJson["uv"].size() >= 2) {
              v.uv = glm::vec2(
                vJson["uv"][0].get<float>(),
                vJson["uv"][1].get<float>()
              );
            } else {
              v.uv = glm::vec2(0.0f, 0.0f);  // Default UV
            }
            v.tangent = glm::vec3(0.0f);
            v.bitangent = glm::vec3(0.0f);
            vertices.push_back(v);
          }
        }
        
        // Parse indices
        if (objJson["indices"].is_array()) {
          for (const auto& idxJson : objJson["indices"]) {
            if (idxJson.is_number_unsigned()) {
              indices.push_back(idxJson.get<unsigned int>());
            }
          }
        }
        
        if (!vertices.empty() && !indices.empty()) {
          // Handle per-face textures if specified
          if (objJson.contains("faces") && objJson["faces"].is_array()) {
            // Create multiple objects, one per face group
            for (const auto& faceJson : objJson["faces"]) {
              if (!faceJson.is_object() || !faceJson.contains("indices") || !faceJson.contains("texture")) {
                continue;
              }
              
              // Get face indices
              std::vector<unsigned int> faceIndices;
              if (faceJson["indices"].is_array()) {
                for (const auto& idxJson : faceJson["indices"]) {
                  if (idxJson.is_number_unsigned()) {
                    faceIndices.push_back(idxJson.get<unsigned int>());
                  }
                }
              }
              
              if (faceIndices.empty()) continue;
              
              // Get texture for this face
              std::string textureName = faceJson["texture"].get<std::string>();
              std::shared_ptr<Texture> faceTexture = nullptr;
              
              // Find texture in textures map
              if (textures_.find(textureName) != textures_.end()) {
                faceTexture = textures_[textureName];
              } else {
                // Fallback to first texture if not found
                if (!objectTextures.empty()) {
                  faceTexture = objectTextures[0];
                }
              }
              
              if (!faceTexture) continue;
              
              // Create mesh input for this face
              input::MeshInput meshInput;
              meshInput.vertices = vertices;
              meshInput.indices = faceIndices;
              meshInput.name = name + "_face_" + textureName;
              meshInput.textures["texture1"] = faceTexture;
              
              auto faceObject = ObjectGenerator::mesh(meshInput);
              
              if (faceObject) {
                // Build model matrix: M = T * R * S
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                
                // Apply scale (non-uniform if vec3, uniform if single value)
                modelMatrix = glm::scale(modelMatrix, scaleVec);
                
                // Apply rotation (Euler angles in degrees: X, Y, Z)
                if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
                  modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis
                  modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis
                  modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis
                }
                
                // Apply translation
                modelMatrix = glm::translate(modelMatrix, position);
                
                faceObject->setModel(modelMatrix);
                
                if (material.has_value()) {
                  faceObject->setMaterial(material.value());
                }
                faceObject->setShader(shader);
                faceObject->visible(visible);
                
                // Parse physics for face object
                if (objJson.contains("physics") && objJson["physics"].is_object()) {
                  auto physicsJson = objJson["physics"];
                  bool physicsEnabled = parseBool(physicsJson, "enabled", false);
                  if (physicsEnabled) {
                    physics::PhysicsObject physicsObject;
                    std::string bodyTypeStr = parseString(physicsJson, "bodyType", "STATIC");
                    
                    if (bodyTypeStr == "STATIC") {
                      physicsObject.bodyType = physics::BodyType::STATIC;
                    } else if (bodyTypeStr == "DYNAMIC") {
                      physicsObject.bodyType = physics::BodyType::DYNAMIC;
                    } else if (bodyTypeStr == "KINEMATIC") {
                      physicsObject.bodyType = physics::BodyType::KINEMATIC;
                    }
                    
                    // For custom geometry, use BOX collider (could be improved to use mesh collider)
                    physicsObject.colliderType = physics::ColliderType::BOX;
                    
                    // Calculate bounding box from vertices
                    glm::vec3 minPos = vertices[0].position;
                    glm::vec3 maxPos = vertices[0].position;
                    for (const auto& v : vertices) {
                      minPos = glm::min(minPos, v.position);
                      maxPos = glm::max(maxPos, v.position);
                    }
                    physicsObject.boundingBox = (maxPos - minPos) * scale;
                    
                    faceObject->physics(physicsObject);
                  }
                }
                
                scene->add(faceObject);
              }
            }
            // Skip creating a single object since we created multiple face objects
            continue;
          } else {
            // Single object with all textures
            input::MeshInput meshInput;
            meshInput.vertices = vertices;
            meshInput.indices = indices;
            meshInput.name = name;
            
            // Use textures from textures array
            for (size_t i = 0; i < objectTextures.size(); ++i) {
              meshInput.textures["texture" + std::to_string(i + 1)] = objectTextures[i];
            }
            
            object = ObjectGenerator::mesh(meshInput);
            
            // Set position, rotation, and scale
            if (object) {
              // Build model matrix: M = T * R * S (applied in reverse order)
              glm::mat4 modelMatrix = glm::mat4(1.0f);
              
              // Apply translation first (will be applied last in matrix multiplication)
              modelMatrix = glm::translate(modelMatrix, position);
              
              // Apply rotation (Euler angles in degrees: X, Y, Z)
              if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
                modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis (roll)
                modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis (yaw)
                modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis (pitch)
              }
              
              // Apply scale last (will be applied first in matrix multiplication)
              modelMatrix = glm::scale(modelMatrix, scaleVec);
              
              object->setModel(modelMatrix);
              
              if (material.has_value()) {
                object->setMaterial(material.value());
              }
              object->setShader(shader);
            }
          }
        }
      }
    } else if (type == "dome") {
      // Dome would need CubeTexture input
      // Skip for now
      continue;
    }
    
    if (object) {
      object->visible(visible);
      
      // Apply rotation and scale if not already applied (for non-mesh objects)
      if (type != "mesh") {
        // Build model matrix: M = T * R * S
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        
        // Apply scale (non-uniform if vec3, uniform if single value)
        modelMatrix = glm::scale(modelMatrix, scaleVec);
        
        // Apply rotation (Euler angles in degrees: X, Y, Z)
        if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
          modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis
          modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis
          modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis
        }
        
        // Apply translation
        modelMatrix = glm::translate(modelMatrix, position);
        
        object->setModel(modelMatrix);
      }
      
      // Parse physics
      if (objJson.contains("physics") && objJson["physics"].is_object()) {
        auto physicsJson = objJson["physics"];
        bool physicsEnabled = parseBool(physicsJson, "enabled", false);
        if (physicsEnabled) {
          std::string bodyTypeStr = parseString(physicsJson, "bodyType", "STATIC");
          std::string colliderTypeStr = parseString(physicsJson, "colliderType", "BOX");
          
          physics::BodyType bodyType = physics::BodyType::STATIC;
          if (bodyTypeStr == "DYNAMIC") bodyType = physics::BodyType::DYNAMIC;
          else if (bodyTypeStr == "KINEMATIC") bodyType = physics::BodyType::KINEMATIC;
          
          physics::ColliderType colliderType = physics::ColliderType::BOX;
          if (colliderTypeStr == "SPHERE") colliderType = physics::ColliderType::SPHERE;
          else if (colliderTypeStr == "PLANE") colliderType = physics::ColliderType::PLANE;
          // Note: MESH collider type is not currently supported
          
          // For planes, create a thin box (thin in Y direction)
          glm::vec3 boundingBox(size, size, size);
          if (colliderType == physics::ColliderType::PLANE) {
            boundingBox = glm::vec3(size, 0.1f, size);  // Thin box for floor
          }
          
          object->physics({
            .isActive = true,
            .bodyType = bodyType,
            .colliderType = colliderType,
            .boundingBox = boundingBox
          });
        }
      }
      
      scene->add(object);
    }
  }
}

void PortalSceneLoader::parsePortals(const nlohmann::json& json) {
  if (!json.is_array()) {
    return;
  }
  
  // First pass: create all portals
  for (const auto& portalJson : json) {
    if (!portalJson.is_object()) continue;
    
    std::string id = parseString(portalJson, "id", "");
    if (id.empty()) {
      std::cerr << "Warning: Portal missing 'id', skipping" << std::endl;
      continue;
    }
    
    auto position = parseVec3(portalJson, "position");
    auto normal = parseVec3(portalJson, "normal", glm::vec3(0.0f, 0.0f, -1.0f));
    float width = parseFloat(portalJson, "width", 2.0f);
    float height = parseFloat(portalJson, "height", 3.0f);
    bool enabled = parseBool(portalJson, "enabled", true);
    bool visible = parseBool(portalJson, "visible", true);
    
    auto portal = std::make_shared<Portal>(position, normal, width, height);
    portal->setEnabled(enabled);
    portal->setVisible(visible);
    
    // Parse framebuffer settings
    if (portalJson.contains("framebuffer") && portalJson["framebuffer"].is_object()) {
      auto fbJson = portalJson["framebuffer"];
      int fbWidth = static_cast<int>(parseFloat(fbJson, "width", 1024.0f));
      int fbHeight = static_cast<int>(parseFloat(fbJson, "height", 1024.0f));
      auto framebuffer = std::make_shared<PortalFramebuffer>(fbWidth, fbHeight);
      portal->setFramebuffer(framebuffer);
    } else {
      // Default framebuffer
      auto framebuffer = std::make_shared<PortalFramebuffer>(1024, 1024);
      portal->setFramebuffer(framebuffer);
    }
    
    portals_[id] = portal;
  }
  
  // Second pass: link portals
  for (const auto& portalJson : json) {
    if (!portalJson.is_object()) continue;
    
    std::string id = parseString(portalJson, "id", "");
    std::string linkedTo = parseString(portalJson, "linkedTo", "");
    
    if (id.empty() || linkedTo.empty()) {
      continue;
    }
    
    if (portals_.find(id) == portals_.end() || portals_.find(linkedTo) == portals_.end()) {
      std::cerr << "Warning: Portal link failed - portal '" << id << "' or '" << linkedTo << "' not found" << std::endl;
      continue;
    }
    
    auto portalA = portals_[id];
    auto portalB = portals_[linkedTo];
    
    // Create portal pair
    auto pair = std::make_shared<PortalPair>(portalA, portalB);
    portalPairs_.push_back(pair);
  }
}

void PortalSceneLoader::parseLights(const nlohmann::json& json, Scene* scene) {
  if (!json.is_array()) {
    return;
  }
  
  for (const auto& lightJson : json) {
    if (!lightJson.is_object()) continue;
    
    std::string type = parseString(lightJson, "type", "");
    bool enabled = parseBool(lightJson, "enabled", true);
    
    if (!enabled) continue;
    
    if (type == "directional") {
      auto direction = parseVec3(lightJson, "direction", glm::vec3(-0.2f, -1.0f, -0.3f));
      auto ambient = parseVec3(lightJson, "ambient", glm::vec3(0.3f, 0.3f, 0.3f));
      auto diffuse = parseVec3(lightJson, "diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
      auto specular = parseVec3(lightJson, "specular", glm::vec3(1.0f, 1.0f, 1.0f));
      
      auto light = std::make_shared<DirectionalLight>(DirectionalLightInput{
        .direction = direction,
        .ambient = ambient,
        .diffuse = diffuse,
        .specular = specular
      });
      scene->add(light);
    } else if (type == "point") {
      auto position = parseVec3(lightJson, "position");
      auto ambient = parseVec3(lightJson, "ambient", glm::vec3(0.3f, 0.3f, 0.3f));
      auto diffuse = parseVec3(lightJson, "diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
      auto specular = parseVec3(lightJson, "specular", glm::vec3(1.0f, 1.0f, 1.0f));
      float constant = parseFloat(lightJson, "constant", 1.0f);
      float linear = parseFloat(lightJson, "linear", 0.09f);
      float quadratic = parseFloat(lightJson, "quadratic", 0.032f);
      
      auto light = std::make_shared<PointLight>(PointLightInput{
        .position = position,
        .ambient = ambient,
        .diffuse = diffuse,
        .specular = specular,
        .constant = constant,
        .linear = linear,
        .quadratic = quadratic
      });
      scene->add(light);
    } else if (type == "spot") {
      auto position = parseVec3(lightJson, "position");
      auto direction = parseVec3(lightJson, "direction", glm::vec3(0.0f, -1.0f, 0.0f));
      auto ambient = parseVec3(lightJson, "ambient", glm::vec3(0.3f, 0.3f, 0.3f));
      auto diffuse = parseVec3(lightJson, "diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
      auto specular = parseVec3(lightJson, "specular", glm::vec3(1.0f, 1.0f, 1.0f));
      float cutOff = parseFloat(lightJson, "cutOff", 12.5f);
      float outerCutOff = parseFloat(lightJson, "outerCutOff", 17.5f);
      float constant = parseFloat(lightJson, "constant", 1.0f);
      float linear = parseFloat(lightJson, "linear", 0.09f);
      float quadratic = parseFloat(lightJson, "quadratic", 0.032f);
      
      auto light = std::make_shared<SpotLight>(SpotLightInput{
        .position = position,
        .direction = direction,
        .ambient = ambient,
        .diffuse = diffuse,
        .specular = specular,
        .cutOff = cutOff,
        .outerCutOff = outerCutOff,
        .constant = constant,
        .linear = linear,
        .quadratic = quadratic
      });
      scene->add(light);
    }
  }
}

void PortalSceneLoader::parseMaterials(const nlohmann::json& json) {
  if (!json.is_object()) {
    return;
  }
  
  for (auto it = json.begin(); it != json.end(); ++it) {
    std::string name = it.key();
    auto materialJson = it.value();
    
    if (!materialJson.is_object()) continue;
    
    Material material;
    material.shininess = parseFloat(materialJson, "shininess", 32.0f);
    
    if (materialJson.contains("diffuse")) {
      auto diffuse = parseVec4(materialJson, "diffuse");
      // Material doesn't have diffuse color in current implementation
    }
    
    if (materialJson.contains("specular")) {
      auto specular = parseVec4(materialJson, "specular");
      // Material doesn't have specular color in current implementation
    }
    
    materials_[name] = material;
  }
}

void PortalSceneLoader::parseTextures(const nlohmann::json& json) {
  if (!json.is_object()) {
    return;
  }
  
  for (auto it = json.begin(); it != json.end(); ++it) {
    std::string name = it.key();
    auto textureJson = it.value();
    
    if (!textureJson.is_object()) continue;
    
    std::string file = parseString(textureJson, "file", "");
    if (file.empty()) continue;
    
    auto texture = std::make_shared<Texture>();
    if (texture->load(file, name)) {
      textures_[name] = texture;
    } else {
      std::cerr << "Warning: Failed to load texture: " << file << std::endl;
    }
  }
}

void PortalSceneLoader::parseCamera(const nlohmann::json& json) {
  cameraConfig_.position = parseVec3(json, "position", glm::vec3(0.0f, 1.5f, 5.0f));
  cameraConfig_.yaw = parseFloat(json, "yaw", -90.0f);
  cameraConfig_.pitch = parseFloat(json, "pitch", 0.0f);
}

// Helper methods
glm::vec3 PortalSceneLoader::parseVec3(const nlohmann::json& json, const std::string& key, glm::vec3 defaultValue) {
  if (!json.contains(key) || !json[key].is_array() || json[key].size() < 3) {
    return defaultValue;
  }
  return glm::vec3(
    json[key][0].get<float>(),
    json[key][1].get<float>(),
    json[key][2].get<float>()
  );
}

glm::vec4 PortalSceneLoader::parseVec4(const nlohmann::json& json, const std::string& key, glm::vec4 defaultValue) {
  if (!json.contains(key) || !json[key].is_array() || json[key].size() < 4) {
    return defaultValue;
  }
  return glm::vec4(
    json[key][0].get<float>(),
    json[key][1].get<float>(),
    json[key][2].get<float>(),
    json[key][3].get<float>()
  );
}

float PortalSceneLoader::parseFloat(const nlohmann::json& json, const std::string& key, float defaultValue) {
  if (!json.contains(key) || !json[key].is_number()) {
    return defaultValue;
  }
  return json[key].get<float>();
}

std::string PortalSceneLoader::parseString(const nlohmann::json& json, const std::string& key, const std::string& defaultValue) {
  if (!json.contains(key) || !json[key].is_string()) {
    return defaultValue;
  }
  return json[key].get<std::string>();
}

bool PortalSceneLoader::parseBool(const nlohmann::json& json, const std::string& key, bool defaultValue) {
  if (!json.contains(key) || !json[key].is_boolean()) {
    return defaultValue;
  }
  return json[key].get<bool>();
}

