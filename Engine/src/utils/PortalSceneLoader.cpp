#include <utils/PortalSceneLoader.h>
#include <system/FileSystem.h>
#include <geometry/Scene.h>
#include <geometry/Portal.h>
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
#include <glad/glad.h>
#include <system/Log.h>

using namespace omega::utils;
using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;
using namespace omega::input;

PortalSceneLoader::PortalSceneLoader() = default;

std::shared_ptr<Texture> PortalSceneLoader::getDefaultWhiteTexture() {
  // Create default white texture lazily using Texture's static factory method
  if (!defaultWhiteTexture_) {
    defaultWhiteTexture_ = Texture::createWhiteTexture();
  }
  return defaultWhiteTexture_;
}

std::shared_ptr<Scene> PortalSceneLoader::loadFromFile(const std::string& filePath) {
  auto jsonString = fs::instance()->string(filePath);
  if (jsonString.empty()) {
    OMEGA_LOG_ERROR("scene-loader", "Failed to load JSON file: {}", filePath);
    return nullptr;
  }
  return loadFromString(jsonString);
}

std::shared_ptr<Scene> PortalSceneLoader::loadFromString(const std::string& jsonString) {
  try {
    auto json = nlohmann::json::parse(jsonString);
    
    // Clear previous state
    portals_.clear();
    textures_.clear();
    materials_.clear();
    
    // Check version
    if (json.contains("version")) {
      std::string version = json["version"];
      if (version != "1.0") {
        OMEGA_LOG_WARN("scene-loader",
                       "JSON version {} may not be fully supported", version);
      }
    }
    
    // Parse scene
    if (json.contains("scene")) {
      parseScene(json["scene"]);
    } else {
      OMEGA_LOG_ERROR("scene-loader", "JSON missing 'scene' object");
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
    
    // Create and configure portal renderer if portals exist. Every portal
    // now goes through the single doorway-based path; portals without a
    // destination are skipped because `renderPortalViewRecursive` has
    // nothing to render for them.
    if (!portals_.empty()) {
      auto portalRenderer = std::make_shared<PortalRenderer>();
      for (auto& [id, portal] : portals_) {
        if (portal->getDestination()) {
          portalRenderer->addPortal(portal);
        }
      }
      portalRenderer->setMaxRecursionDepth(3);  // Default recursion depth
      portalRenderer->setEnabled(true);
      scene->setPortalRenderer(portalRenderer);
    }
    
    // Prepare scene (sets up physics)
    scene->prepare();
    
    return scene;
  } catch (const nlohmann::json::exception& e) {
    OMEGA_LOG_ERROR("scene-loader", "JSON parsing error: {}", e.what());
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
          
          // Parse vertices and apply scale directly to vertex positions
          // This ensures the scale is applied correctly regardless of vertex coordinate ranges
          if (objJson["vertices"].is_array()) {
            for (const auto& vJson : objJson["vertices"]) {
              Vertex v;
              if (vJson.contains("position") && vJson["position"].is_array() && vJson["position"].size() >= 3) {
                glm::vec3 pos(
                  vJson["position"][0].get<float>(),
                  vJson["position"][1].get<float>(),
                  vJson["position"][2].get<float>()
                );
                // Apply scale directly to vertex positions
                v.position = pos * scaleVec;
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
          // Handle per-face textures/materials if specified
          if (objJson.contains("faces") && objJson["faces"].is_array()) {
            // Create multiple objects, one per face group
            for (const auto& faceJson : objJson["faces"]) {
              // Face must have indices, and either texture or material
              if (!faceJson.is_object() || !faceJson.contains("indices") || 
                  (!faceJson.contains("texture") && !faceJson.contains("material"))) {
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
              
              // Get texture for this face (optional - can use object default textures)
              std::shared_ptr<Texture> faceTexture = nullptr;
              std::string faceIdentifier = "face";
              
              if (faceJson.contains("texture") && faceJson["texture"].is_string()) {
                std::string textureName = faceJson["texture"].get<std::string>();
                faceIdentifier = textureName;
                
                // Find texture in textures map
                if (textures_.find(textureName) != textures_.end()) {
                  faceTexture = textures_[textureName];
                } else {
                  OMEGA_LOG_WARN("scene-loader",
                                 "Face texture '{}' not found", textureName);
                }
              }
              
              // If no face-specific texture, use object's default textures
              if (!faceTexture) {
                if (!objectTextures.empty()) {
                  faceTexture = objectTextures[0];
                } else {
                  // No texture available - use default white texture (for material-only rendering)
                  // The shader will multiply this white texture by the material color
                  faceTexture = getDefaultWhiteTexture();
                  if (!faceTexture) {
                    OMEGA_LOG_WARN("scene-loader",
                                   "Could not create default white texture, skipping face");
                    continue;
                  }
                }
              }
              
              // Get material for this face (optional, overrides object-level material)
              std::optional<Material> faceMaterial = material;
              if (faceJson.contains("material") && faceJson["material"].is_string()) {
                std::string materialName = faceJson["material"].get<std::string>();
                faceIdentifier = materialName;  // Use material name as identifier
                
                // Look up material in materials map
                if (materials_.find(materialName) != materials_.end()) {
                  faceMaterial = materials_[materialName];
                } else {
                  OMEGA_LOG_WARN("scene-loader",
                                 "Face material '{}' not found, using object material",
                                 materialName);
                }
              }
              
              // Create mesh input for this face
              input::MeshInput meshInput;
              meshInput.vertices = vertices;
              meshInput.indices = faceIndices;
              meshInput.name = name + "_face_" + faceIdentifier;
              meshInput.textures["texture1"] = faceTexture;
              
              auto faceObject = ObjectGenerator::mesh(meshInput);
              
              if (faceObject) {
                // Build model matrix: M = T * R
                // Scale is already applied to vertex positions, so we only need translation and rotation
                glm::mat4 rotationMatrix = glm::mat4(1.0f);
                if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
                  rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis
                  rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis
                  rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis
                }
                
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
                
                // Multiply in order: T * R (scale already applied to vertices)
                glm::mat4 modelMatrix = translationMatrix * rotationMatrix;
                
                faceObject->setModel(modelMatrix);
                
                // Apply face-specific material if available, otherwise use object material
                if (faceMaterial.has_value()) {
                  faceObject->setMaterial(faceMaterial.value());
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
                    // Vertices already have scale applied, so use them directly
                    glm::vec3 minPos = vertices[0].position;
                    glm::vec3 maxPos = vertices[0].position;
                    for (const auto& v : vertices) {
                      minPos = glm::min(minPos, v.position);
                      maxPos = glm::max(maxPos, v.position);
                    }
                    // Vertices already scaled, so bounding box is just the size
                    physicsObject.boundingBox = (maxPos - minPos);
                    
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
            
            // Set position and rotation
            // Scale is already applied to vertex positions, so we only need translation and rotation
            if (object) {
              // Build model matrix: M = T * R
              // Build matrices separately and multiply in correct order
              glm::mat4 rotationMatrix = glm::mat4(1.0f);
              if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
                rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis
                rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis
                rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis
              }
              
              glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
              
              // Multiply in order: T * R (scale already applied to vertices)
              glm::mat4 modelMatrix = translationMatrix * rotationMatrix;
              
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
        // Build matrices separately and multiply in correct order
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleVec);
        
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
          rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X-axis
          rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis
          rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-axis
        }
        
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
        
        // Multiply in order: T * R * S
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        
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
      OMEGA_LOG_WARN("scene-loader", "Portal missing 'id', skipping");
      continue;
    }
    
    // Parse position
    auto position = parseVec3(portalJson, "position");
    
    // Parse normal (legacy) or calculate from transform (new system)
    glm::vec3 normal = parseVec3(portalJson, "normal", glm::vec3(0.0f, 0.0f, -1.0f));
    
    // Parse transform (new system) - if present, use it to calculate normal
    if (portalJson.contains("transform") && portalJson["transform"].is_object()) {
      auto transformJson = portalJson["transform"];
      if (transformJson.contains("rotation") && transformJson["rotation"].is_array()) {
        auto rotation = parseVec3(transformJson, "rotation");
        // Convert Euler angles to normal vector
        // Assuming rotation is in degrees: [pitch, yaw, roll] or [x, y, z]
        float pitch = glm::radians(rotation.x);
        float yaw = glm::radians(rotation.y);
        
        // Calculate normal from rotation (assuming portal faces -Z by default)
        normal.x = -sin(yaw) * cos(pitch);
        normal.y = sin(pitch);
        normal.z = -cos(yaw) * cos(pitch);
        normal = glm::normalize(normal);
      }
    }
    
    float width = parseFloat(portalJson, "width", 2.0f);
    float height = parseFloat(portalJson, "height", 3.0f);
    bool enabled = parseBool(portalJson, "enabled", true);
    bool visible = parseBool(portalJson, "visible", true);
    
    auto portal = std::make_shared<Portal>(position, normal, width, height);
    portal->setEnabled(enabled);
    portal->setVisible(visible);
    
    // Parse new doorway-based properties
    std::string type = parseString(portalJson, "type", "doorway");
    bool passable = parseBool(portalJson, "passable", true);
    bool open = parseBool(portalJson, "open", true);
    bool mirrorOverlay = parseBool(portalJson, "mirrorOverlay", false);
    float mirrorIntensity = parseFloat(portalJson, "mirrorIntensity", 0.5f);
    // Phase 1.5 addition: per-portal tint applied by portal.fs when the
    // overlay is enabled. Defaults to white so omitting the field keeps the
    // overlay visually neutral (still takes the intensity mix, but base.rgb
    // * vec3(1) is a no-op).
    glm::vec3 mirrorTint = parseVec3(portalJson, "mirrorTint", glm::vec3(1.0f));

    portal->setPassable(passable);
    portal->setOpen(open);
    portal->setMirrorOverlay(mirrorOverlay, mirrorIntensity, mirrorTint);
    
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
  
  // Second pass: link portals (support both old and new formats)
  for (const auto& portalJson : json) {
    if (!portalJson.is_object()) continue;
    
    std::string id = parseString(portalJson, "id", "");
    if (id.empty()) {
      continue;
    }
    
    if (portals_.find(id) == portals_.end()) {
      continue;
    }
    
    auto portal = portals_[id];
    
    // Doorway-based destination. `destination == id` = self-linked mirror.
    // A portal with no `destination` field is enabled but never traversed —
    // the renderer will skip it in `renderPortalViewRecursive`.
    const std::string destinationId = parseString(portalJson, "destination", "");
    if (!destinationId.empty()) {
      if (destinationId == id) {
        portal->setDestination(portal);
      } else if (portals_.find(destinationId) != portals_.end()) {
        portal->setDestination(portals_[destinationId]);
      } else {
        OMEGA_LOG_WARN("scene-loader",
                       "Portal destination '{}' not found for portal '{}'",
                       destinationId, id);
      }
    }
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
    
    // Parse shininess
    material.shininess = parseFloat(materialJson, "shininess", 32.0f);
    
    // Parse color (RGBA)
    if (materialJson.contains("color") && materialJson["color"].is_array()) {
      auto color = parseVec4(materialJson, "color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
      material.color = color;
    }
    
    // Parse diffuse (RGB or RGBA - convert to RGB)
    if (materialJson.contains("diffuse")) {
      if (materialJson["diffuse"].is_array()) {
        auto diffuseArray = materialJson["diffuse"];
        if (diffuseArray.size() >= 3) {
          material.diffuse = glm::vec3(
            diffuseArray[0].get<float>(),
            diffuseArray[1].get<float>(),
            diffuseArray[2].get<float>()
          );
        }
        // If color was not set, use diffuse as color (with alpha from opacity or 1.0)
        if (materialJson.contains("opacity")) {
          material.color = glm::vec4(material.diffuse, parseFloat(materialJson, "opacity", 1.0f));
        } else if (!materialJson.contains("color")) {
          material.color = glm::vec4(material.diffuse, 1.0f);
        }
      }
    }
    
    // Parse opacity
    if (materialJson.contains("opacity")) {
      material.opacity = parseFloat(materialJson, "opacity", 1.0f);
      // Update color alpha if color was set
      material.color.a = material.opacity;
    }
    
    // Parse specular (for future use - currently Material only stores specular texture)
    if (materialJson.contains("specular")) {
      // Material doesn't have specular color in current implementation
      // This could be added in the future if needed
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
      OMEGA_LOG_WARN("scene-loader", "Failed to load texture: {}", file);
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

