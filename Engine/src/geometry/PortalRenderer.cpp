#include <geometry/PortalRenderer.h>
#include <geometry/Portal.h>
#include <geometry/PortalPair.h>
#include <geometry/PortalSurface.h>
#include <geometry/Scene.h>
#include <render/Camera.h>
#include <render/PortalFramebuffer.h>
#include <render/PortalCamera.h>
#include <render/PortalViewCamera.h>
#include <render/Shader.h>
#include <render/Texture.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace omega::geometry;
using namespace omega::render;

PortalRenderer::PortalRenderer() = default;

void PortalRenderer::addPortalPair(std::shared_ptr<PortalPair> portalPair) {
  if (portalPair && portalPair->isValid()) {
    portalPairs_.push_back(portalPair);
  }
}

void PortalRenderer::clearPortals() {
  portalPairs_.clear();
}

void PortalRenderer::renderPortals(std::shared_ptr<Scene> scene,
                                   std::shared_ptr<Camera> playerCamera,
                                   std::shared_ptr<Shader> portalShader) {
  if (!enabled_ || !scene || !playerCamera) {
    return;
  }

  // Render portal views first (to framebuffers)
  // This happens BEFORE the main scene so we can use the framebuffers when rendering surfaces
  for (auto& portalPair : portalPairs_) {
    if (!portalPair->isEnabled()) {
      continue;
    }

    auto portalA = portalPair->getPortalA();
    auto portalB = portalPair->getPortalB();

    if (!portalA || !portalB) {
      continue;
    }

    // Render portal A's view (what you see through portal A)
    if (isPortalVisible(portalA, playerCamera) && portalA->isEnabled()) {
      renderPortalView(portalA, portalB, scene, playerCamera, 0);
    }

    // Render portal B's view (what you see through portal B)
    if (isPortalVisible(portalB, playerCamera) && portalB->isEnabled()) {
      renderPortalView(portalB, portalA, scene, playerCamera, 0);
    }
  }
  
  // Ensure viewport is restored to window size after portal view rendering
  GLFWwindow* currentWindow = glfwGetCurrentContext();
  if (currentWindow) {
    int width, height;
    glfwGetFramebufferSize(currentWindow, &width, &height);
    glViewport(0, 0, width, height);
  }
}

void PortalRenderer::renderPortalSurfaces(std::shared_ptr<Camera> playerCamera,
                                          std::shared_ptr<Shader> portalShader) {
  if (!enabled_ || !playerCamera) {
    return;
  }

  // Render portal surfaces AFTER the main scene so they appear on top
  static bool firstCall = true;
  if (firstCall) {
    std::cout << "[Portal] Rendering " << portalPairs_.size() << " portal pair(s)" << std::endl;
    firstCall = false;
  }
  
  for (auto& portalPair : portalPairs_) {
    if (!portalPair->isEnabled()) {
      continue;
    }

    auto portalA = portalPair->getPortalA();
    auto portalB = portalPair->getPortalB();

    if (portalA && portalA->isVisible() && portalA->isEnabled()) {
      static int renderCountA = 0;
      if (renderCountA++ < 5) {
        glm::vec3 posA = portalA->getPosition();
        std::cout << "[Portal] Rendering portal A at (" << posA.x << ", " << posA.y << ", " << posA.z << ")" << std::endl;
      }
      renderPortalSurface(portalA, playerCamera, portalShader);
    }

    if (portalB && portalB->isVisible() && portalB->isEnabled()) {
      static int renderCountB = 0;
      if (renderCountB++ < 5) {
        glm::vec3 posB = portalB->getPosition();
        std::cout << "[Portal] Rendering portal B at (" << posB.x << ", " << posB.y << ", " << posB.z << ")" << std::endl;
      }
      renderPortalSurface(portalB, playerCamera, portalShader);
    }
  }
}

void PortalRenderer::renderPortalView(std::shared_ptr<Portal> sourcePortal,
                                     std::shared_ptr<Portal> destPortal,
                                     std::shared_ptr<Scene> scene,
                                     std::shared_ptr<Camera> playerCamera,
                                     int recursionDepth) {
  if (!sourcePortal || !destPortal || !scene || !playerCamera) {
    return;
  }

  // Prevent infinite recursion
  if (recursionDepth >= maxRecursionDepth_) {
    return;
  }

  auto framebuffer = sourcePortal->getFramebuffer();
  if (!framebuffer || !framebuffer->isComplete()) {
    return;
  }

  // Bind portal framebuffer
  framebuffer->bind();
  // Clear to a visible color to test framebuffer rendering
  // Use a blue-green color so we can see if framebuffer is working
  framebuffer->clear(0.2f, 0.5f, 0.8f, 1.0f);  // Blue-green background

  // Calculate portal camera view matrix
  glm::mat4 portalView = PortalCamera::calculatePortalView(
      *playerCamera, *sourcePortal, *destPortal);

  // Create temporary camera with portal view
  auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);
  
  // Fix aspect ratio: framebuffer might be square (1024x1024) but window is 16:9
  // Create a projection matrix that matches the framebuffer's aspect ratio
  float framebufferAspect = static_cast<float>(framebuffer->getWidth()) / static_cast<float>(framebuffer->getHeight());
  glm::mat4 baseProjection = playerCamera->projectionMatrix();
  
  // Extract FOV and near/far from base projection (assuming perspective)
  // For perspective matrices, we can extract the FOV from the top component
  float fov = 45.0f; // Default, will be extracted if possible
  float nearPlane = 0.1f;
  float farPlane = 100.0f;
  
  // Try to extract from base camera if it's a CameraFPS or similar
  // For now, create a new projection with correct aspect ratio
  // We'll use the same FOV as the base camera but adjust aspect
  portalCamera->setPerspective(45.0f, 
                               static_cast<float>(framebuffer->getWidth()),
                               static_cast<float>(framebuffer->getHeight()),
                               nearPlane, farPlane);

  // Render scene from portal perspective
  // Note: This renders the scene objects, but NOT portals recursively (to avoid infinite recursion)
  static int renderCount = 0;
  if (renderCount++ < 3) {
    glm::vec3 camPos = portalCamera->position();
    glm::vec3 playerPos = playerCamera->position();
    glm::vec3 sourcePos = sourcePortal->getPosition();
    glm::vec3 destPos = destPortal->getPosition();
    glm::vec3 camFront = portalCamera->front();
    glm::mat4 view = portalCamera->viewMatrix();
    glm::mat4 proj = portalCamera->projectionMatrix();
    std::cout << "[Portal] Rendering view:" << std::endl;
    std::cout << "  Player pos: (" << playerPos.x << "," << playerPos.y << "," << playerPos.z << ")" << std::endl;
    std::cout << "  Source portal pos: (" << sourcePos.x << "," << sourcePos.y << "," << sourcePos.z << ")" << std::endl;
    std::cout << "  Dest portal pos: (" << destPos.x << "," << destPos.y << "," << destPos.z << ")" << std::endl;
    std::cout << "  Portal camera pos: (" << camPos.x << "," << camPos.y << "," << camPos.z << ")" << std::endl;
    std::cout << "  Portal camera front: (" << camFront.x << "," << camFront.y << "," << camFront.z << ")" << std::endl;
    std::cout << "  Framebuffer size: " << framebuffer->getWidth() << "x" << framebuffer->getHeight() << std::endl;
  }
  
  // Ensure we're rendering to the framebuffer
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error before scene render: " << err << std::endl;
  }
  
  // Check framebuffer status
  GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[Portal] Framebuffer not complete! Status: " << fbStatus << std::endl;
  }
  
  scene->render(portalCamera);
  
  // Check for errors after rendering
  err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error after scene render: " << err << std::endl;
  }

  // Unbind framebuffer
  framebuffer->unbind();
}

void PortalRenderer::renderPortalSurface(std::shared_ptr<Portal> portal,
                                        std::shared_ptr<Camera> playerCamera,
                                        std::shared_ptr<Shader> portalShader) {
  if (!portal || !playerCamera) {
    return;
  }

  auto framebuffer = portal->getFramebuffer();
  if (!framebuffer || !framebuffer->isComplete()) {
    return;
  }

  // Create portal shader if not provided or if wrong shader type
  // We need a simple shader, not the complex lighting shader
  static std::shared_ptr<Shader> defaultPortalShader = nullptr;
  if (!defaultPortalShader) {
    // Try loading portal shaders from file system first, then from zip
    // Try: ./portal.vs + ./portal.fs
    defaultPortalShader = Shader::fromFile(4, 2, "./portal.vs", "./portal.fs");
    if (!defaultPortalShader) {
      // Try: :/shaders/portal.vs + :/shaders/portal.fs
      defaultPortalShader = Shader::fromFile(4, 2, ":/shaders/portal.vs", ":/shaders/portal.fs");
    }
    if (!defaultPortalShader) {
      // Fallback: try with core.vs if portal.vs doesn't exist
      defaultPortalShader = Shader::fromFile(4, 2, ":/shaders/core.vs", "./portal.fs");
      if (!defaultPortalShader) {
        defaultPortalShader = Shader::fromFile(4, 2, ":/shaders/core.vs", ":/shaders/portal.fs");
      }
    }
    if (defaultPortalShader) {
      defaultPortalShader->setInt("portalTexture", 0);
      // Verify shader is valid by checking for GL errors
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) {
        std::cerr << "[Portal] WARNING: GL Error after shader load: " << err << std::endl;
      }
      std::cout << "[Portal] Shader loaded successfully" << std::endl;
    } else {
      std::cerr << "[Portal] ERROR: Failed to load portal shader!" << std::endl;
      std::cerr << "[Portal] Tried: ./portal.vs+./portal.fs, :/shaders/portal.vs+:/shaders/portal.fs, and fallbacks" << std::endl;
      return;  // Can't render without shader
    }
  }
  
  // Always use the portal shader, not the mesh shader
  portalShader = defaultPortalShader;

  if (!portalShader) {
    std::cerr << "[Portal] ERROR: Portal shader is null!" << std::endl;
    return;  // Need shader to render portal surface
  }

  // Get or create portal surface object
  std::shared_ptr<Object> portalSurface;
  auto it = portalSurfaces_.find(portal);
  if (it == portalSurfaces_.end()) {
    // Create portal surface mesh
    portalSurface = PortalSurface::createSurface(portal, portalShader);
    if (portalSurface) {
      portalSurfaces_[portal] = portalSurface;
    } else {
      return;
    }
  } else {
    portalSurface = it->second;
  }

  if (!portalSurface) {
    return;
  }

  // Render portal surface directly (bypass Object::render() which expects lighting shader)
  // Get the portal surface's VAO and count
  unsigned int vao = portalSurface->getVAO();
  unsigned int count = portalSurface->getCount();
  ObjectType type = portalSurface->getType();
  
  static int debugRenderCount = 0;
  if (debugRenderCount++ < 3) {
    glm::vec3 portalPos = portal->getPosition();
    glm::mat4 model = portalSurface->getModel();
    glm::vec3 camPos = playerCamera->position();
    glm::vec3 portalNormal = portal->getNormal();
    
    // Check if camera is in front of portal
    bool inFront = portal->isPointInFront(camPos);
    
    std::cout << "[Portal] Rendering surface: vao=" << vao << ", count=" << count 
              << ", portalPos=(" << portalPos.x << "," << portalPos.y << "," << portalPos.z << ")"
              << ", camPos=(" << camPos.x << "," << camPos.y << "," << camPos.z << ")"
              << ", inFront=" << (inFront ? "YES" : "NO")
              << ", textureId=" << framebuffer->getColorTexture() << std::endl;
    
    // Print model matrix translation
    std::cout << "[Portal] Model matrix translation: (" << model[3][0] << ", " << model[3][1] << ", " << model[3][2] << ")" << std::endl;
  }
  
  if (vao == 0 || count == 0) {
    std::cerr << "[Portal] ERROR: Invalid VAO=" << vao << " or count=" << count << std::endl;
    return;
  }
  
  if (!portalShader) {
    std::cerr << "[Portal] ERROR: Portal shader is null!" << std::endl;
    return;
  }
  
  // Bind framebuffer texture
  unsigned int textureId = framebuffer->getColorTexture();
  if (textureId == 0) {
    std::cerr << "[Portal] ERROR: Framebuffer texture ID is 0!" << std::endl;
    return;
  }
  
  // Set up portal shader FIRST - keep it active throughout rendering
  portalShader->use();
  
  // Check for GL errors after use()
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error after shader->use(): " << err << std::endl;
    return;
  }
  
  // Get the currently active program ID
  GLint currentProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  if (currentProgram == 0) {
    std::cerr << "[Portal] ERROR: No shader program is active!" << std::endl;
    return;
  }
  
  // Then bind texture and set uniform
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureId);
  GLint texLoc = glGetUniformLocation(currentProgram, "portalTexture");
  if (texLoc >= 0) {
    glUniform1i(texLoc, 0);
  } else {
    std::cerr << "[Portal] WARNING: portalTexture uniform not found in shader!" << std::endl;
  }
  
  // Set matrices directly (bypass setMat4fv which calls unuse)
  glm::mat4 projection = playerCamera->projectionMatrix();
  glm::mat4 view = playerCamera->viewMatrix();
  glm::mat4 model = portalSurface->getModel();
  
  GLint projLoc = glGetUniformLocation(currentProgram, "projection");
  GLint viewLoc = glGetUniformLocation(currentProgram, "view");
  GLint modelLoc = glGetUniformLocation(currentProgram, "model");
  
  if (projLoc >= 0) {
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
  } else {
    std::cerr << "[Portal] WARNING: projection uniform not found!" << std::endl;
  }
  if (viewLoc >= 0) {
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
  } else {
    std::cerr << "[Portal] WARNING: view uniform not found!" << std::endl;
  }
  if (modelLoc >= 0) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  } else {
    std::cerr << "[Portal] WARNING: model uniform not found!" << std::endl;
  }
  
  // Check for GL errors after setting uniforms
  err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error after setting uniforms: " << err << std::endl;
    return;
  }
  
  // Enable depth testing (portals should respect depth)
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  
  // Render the portal quad
  glBindVertexArray(vao);
  
  // Check for GL errors after binding VAO
  err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error after binding VAO: " << err << std::endl;
    glBindVertexArray(0);
    return;
  }
  
  if (type == ObjectType::Elements) {
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, count);
  }
  glBindVertexArray(0);
  
  // Check for GL errors after rendering
  err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "[Portal] GL Error after draw: " << err << " (0x" << std::hex << err << std::dec << ")" << std::endl;
  }
  
  // Restore depth testing and writing
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  
  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);
}

bool PortalRenderer::isPortalVisible(std::shared_ptr<Portal> portal,
                                    std::shared_ptr<Camera> camera) const {
  if (!portal || !camera) {
    return false;
  }

  // Simple visibility check: is camera in front of portal?
  glm::vec3 cameraPos = camera->position();
  return portal->isPointInFront(cameraPos);
}

