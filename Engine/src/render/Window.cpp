#include <render/Window.h>
#include <render/Texture.h>

#include <glad/glad.h>
#include <chrono>
#include <iostream>
#include <iso646.h>
#include <GLFW/glfw3.h>

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;

std::shared_ptr<OWindow> mainWindow = nullptr;

static bool firstMouse{true};
static float lastX{0};
static float lastY{0};

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  mainWindow->mouseWheelEvent(xoffset, yoffset);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos;  // reversed since y-coordinates go from bottom to top
  lastX = xpos;
  lastY = ypos;

  mainWindow->mouseMoveEvent(xoffset, yoffset);
}

OWindow::OWindow(int width, int height, int flags) {
  mainWindow = std::shared_ptr<OWindow>(this);

  m_width = width;
  m_height = height;

  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

  m_time = duration.count();

  initGL();
}

OWindow::~OWindow() {}

void OWindow::keyEvent(int type, int state, int key, bool repeat) {}

std::shared_ptr<OWindow> OWindow::instance() { return mainWindow; }

void OWindow::mouseWheelEvent(float xoffset, float yoffset) {
  if (camera) camera->processMouseScroll(yoffset);
}

void OWindow::mouseMoveEvent(float xoffset, float yoffset) {
  if (camera) camera->processMouseMovement(xoffset, yoffset);
}

void OWindow::mouseButtonEvent(int, int, int) {}

void OWindow::setCamera(std::shared_ptr<Camera> _camera) {
  camera = _camera;
  camera->updateCameraVectors();
  camera->setPerspective(45.f, m_width, m_height, 0.01f, 300.f);
}

void OWindow::quit() { m_quit = true; }

bool OWindow::isRuning() { return not m_quit; }

void OWindow::clear() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool OWindow::render() { return true; }

void OWindow::swap() { glfwSwapBuffers(m_window); }

void OWindow::process() {
  calculateDuration();
  camera->updateShader();
  processEvents();
}

void OWindow::calculateDuration() {
  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

  m_deltaTime = (duration.count() - m_time) / 1000.f;
  m_time = duration.count();

  //    std::cout << "FPS => " << 1.f / m_deltaTime << std::endl;
}

void OWindow::processEvents() {
  glfwPollEvents();

  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) m_quit = true;

  float cameraSpeed = static_cast<float>(2.5 * m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    camera->processKeyboard(FORWARD, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
    camera->processKeyboard(BACKWARD, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
    camera->processKeyboard(LEFT, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
    camera->processKeyboard(RIGHT, m_deltaTime);
}

bool OWindow::isOpen() { return true; }

bool OWindow::isVisible() {
  // Is the window open and visible, ie. not minimized?
  if (!m_window) return false;

  return false;
}

bool OWindow::isFocused() {
  return glfwGetWindowAttrib(m_window, GLFW_FOCUSED) == GLFW_TRUE;
}

bool OWindow::isMinimized() {
  return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
}

bool OWindow::isMaximized() {
  return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

unsigned int OWindow::getWindowId() { return m_windowId; }

void OWindow::setFocus() {
  glfwSetWindowAttrib(m_window, GLFW_FOCUSED, GLFW_TRUE);
}

void OWindow::minimize() { glfwIconifyWindow(m_window); }

void OWindow::maximize() { glfwMaximizeWindow(m_window); }

void OWindow::restore() { glfwRestoreWindow(m_window); }

void OWindow::hide() { glfwHideWindow(m_window); }

void OWindow::show() { glfwShowWindow(m_window); }

void OWindow::close() { delete this; }

bool OWindow::isFullscreen() { return m_fullscreen; }

void OWindow::setFullscreen(const bool fullscreen) {
  if (fullscreen)
    glfwSetWindowMonitor(m_window, glfwGetPrimaryMonitor(), 0, 0, m_width,
                         m_height, GLFW_DONT_CARE);
  else
    glfwSetWindowMonitor(m_window, NULL, 0, 0, m_width, m_height,
                         GLFW_DONT_CARE);
}

bool OWindow::setSize(int width, int height) {
  m_width = width;
  m_height = height;

  glfwSetWindowSize(m_window, width, height);
  glViewport(0, 0, m_width, m_height);

  if (camera)
    camera->setPerspective(45.0f, (float)m_width, (float)m_height, 0.125f,
                           512.0f);

  return true;
}

bool OWindow::initGL() {
  glfwSetErrorCallback([](int, const char* desc) {
    std::cerr << desc << "\n";
    std::exit(EXIT_FAILURE);
  });
  if (!glfwInit()) {
    std::cout << "GLFW failed to Initialize!" << std::endl;
    return -1;
  }

  GLfloat vertices[] = {-0.5f, -0.5f * float(sqrt(3)) / 3,    0.0f,
                        0.5f,  -0.5f * float(sqrt(3)) / 3,    0.0f,
                        0.0f,  0.5f * float(sqrt(3)) * 2 / 3, 0.0f};

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  m_window = glfwCreateWindow(m_width, m_height, "Open GL", NULL, NULL);
  if (!m_window) {
    std::cout
        << "Something went Wrong when Creating a Window!\nShutting down ..."
        << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(m_window);
  glfwSetCursorPosCallback(m_window, mouse_callback);
  glfwSetScrollCallback(m_window, scroll_callback);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glViewport(0, 0, m_width, m_height);

  bool success = true;
  GLenum error = GL_NO_ERROR;

  std::cout << glGetString(GL_VERSION) << std::endl;

  // Initialize clear color
  glClearColor(0.f, 0.f, 0.f, 1.f);
  // Check for error
  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cout << "Error: glClearColor" << std::endl;
    success = false;
  }
  glViewport(0, 0, m_width, m_height);

  glEnable(GL_DEPTH_TEST);
  //    glEnable(GL_CULL_FACE);

  return success;
}
