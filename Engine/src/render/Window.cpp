#include <render/Window.h>
#include <render/Texture.h>

#include <glad/glad.h>
#include <chrono>
#include <iostream>
#include <GLFW/glfw3.h>

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;

std::shared_ptr<Window> mainWindow = nullptr;
static int _lastState = 0;

static bool firstMouse{true};
static float lastX{0};
static float lastY{0};

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  mainWindow->mouseWheelEvent(xoffset, yoffset);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
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

static inline int translateState(int action) {
  switch (action) {
  case GLFW_PRESS:return Window::KEY_STATE_DOWN;
  case GLFW_RELEASE:return Window::KEY_STATE_UP;
  case GLFW_REPEAT:return Window::KEY_STATE_REPEAT;
  default:return Window::KEY_STATE_UP;
  }
}
static inline int translateKey(int key) {
  switch (key) {
  case GLFW_KEY_SPACE:return Window::KEY_SPACE;
  case GLFW_KEY_APOSTROPHE:return Window::KEY_APOSTROPHE;
  case GLFW_KEY_COMMA:return Window::KEY_COMMA;
  case GLFW_KEY_MINUS:return Window::KEY_MINUS;
  case GLFW_KEY_PERIOD:return Window::KEY_PERIOD;
  case GLFW_KEY_SLASH:return Window::KEY_SLASH;
  case GLFW_KEY_0:return Window::KEY_0;
  case GLFW_KEY_1:return Window::KEY_1;
  case GLFW_KEY_2:return Window::KEY_2;
  case GLFW_KEY_3:return Window::KEY_3;
  case GLFW_KEY_4:return Window::KEY_4;
  case GLFW_KEY_5:return Window::KEY_5;
  case GLFW_KEY_6:return Window::KEY_6;
  case GLFW_KEY_7:return Window::KEY_7;
  case GLFW_KEY_8:return Window::KEY_8;
  case GLFW_KEY_9:return Window::KEY_9;
  case GLFW_KEY_SEMICOLON:return Window::KEY_SEMICOLON;
  case GLFW_KEY_EQUAL:return Window::KEY_EQUAL;
  case GLFW_KEY_A:return Window::KEY_A;
  case GLFW_KEY_B:return Window::KEY_B;
  case GLFW_KEY_C:return Window::KEY_C;
  case GLFW_KEY_D:return Window::KEY_D;
  case GLFW_KEY_E:return Window::KEY_E;
  case GLFW_KEY_F:return Window::KEY_F;
  case GLFW_KEY_G:return Window::KEY_G;
  case GLFW_KEY_H:return Window::KEY_H;
  case GLFW_KEY_I:return Window::KEY_I;
  case GLFW_KEY_J:return Window::KEY_J;
  case GLFW_KEY_K:return Window::KEY_K;
  case GLFW_KEY_L:return Window::KEY_L;
  case GLFW_KEY_M:return Window::KEY_M;
  case GLFW_KEY_N:return Window::KEY_N;
  case GLFW_KEY_O:return Window::KEY_O;
  case GLFW_KEY_P:return Window::KEY_P;
  case GLFW_KEY_Q:return Window::KEY_Q;
  case GLFW_KEY_R:return Window::KEY_R;
  case GLFW_KEY_S:return Window::KEY_S;
  case GLFW_KEY_T:return Window::KEY_T;
  case GLFW_KEY_U:return Window::KEY_U;
  case GLFW_KEY_V:return Window::KEY_V;
  case GLFW_KEY_W:return Window::KEY_W;
  case GLFW_KEY_X:return Window::KEY_X;
  case GLFW_KEY_Y:return Window::KEY_Y;
  case GLFW_KEY_Z:return Window::KEY_Z;
  case GLFW_KEY_LEFT_BRACKET:return Window::KEY_LEFT_BRACKET;
  case GLFW_KEY_BACKSLASH:return Window::KEY_BACKSLASH;
  case GLFW_KEY_RIGHT_BRACKET:return Window::KEY_RIGHT_BRACKET;
  case GLFW_KEY_GRAVE_ACCENT:return Window::KEY_GRAVE_ACCENT;
  case GLFW_KEY_WORLD_1:return Window::KEY_WORLD_1;
  case GLFW_KEY_WORLD_2:return Window::KEY_WORLD_2;
  case GLFW_KEY_ESCAPE:return Window::KEY_ESCAPE;
  case GLFW_KEY_ENTER:return Window::KEY_ENTER;
  case GLFW_KEY_TAB:return Window::KEY_TAB;
  case GLFW_KEY_BACKSPACE:return Window::KEY_BACKSPACE;
  case GLFW_KEY_INSERT:return Window::KEY_INSERT;
  case GLFW_KEY_DELETE:return Window::KEY_DELETE;
  case GLFW_KEY_RIGHT:return Window::KEY_RIGHT;
  case GLFW_KEY_LEFT:return Window::KEY_LEFT;
  case GLFW_KEY_DOWN:return Window::KEY_DOWN;
  case GLFW_KEY_UP:return Window::KEY_UP;
  case GLFW_KEY_PAGE_UP:return Window::KEY_PAGE_UP;
  case GLFW_KEY_PAGE_DOWN:return Window::KEY_PAGE_DOWN;
  case GLFW_KEY_HOME:return Window::KEY_HOME;
  case GLFW_KEY_END:return Window::KEY_END;
  case GLFW_KEY_CAPS_LOCK:return Window::KEY_CAPS_LOCK;
  case GLFW_KEY_SCROLL_LOCK:return Window::KEY_SCROLL_LOCK;
  case GLFW_KEY_NUM_LOCK:return Window::KEY_NUM_LOCK;
  case GLFW_KEY_PRINT_SCREEN:return Window::KEY_PRINT_SCREEN;
  case GLFW_KEY_PAUSE:return Window::KEY_PAUSE;
  case GLFW_KEY_F1:return Window::KEY_F1;
  case GLFW_KEY_F2:return Window::KEY_F2;
  case GLFW_KEY_F3:return Window::KEY_F3;
  case GLFW_KEY_F4:return Window::KEY_F4;
  case GLFW_KEY_F5:return Window::KEY_F5;
  case GLFW_KEY_F6:return Window::KEY_F6;
  case GLFW_KEY_F7:return Window::KEY_F7;
  case GLFW_KEY_F8:return Window::KEY_F8;
  case GLFW_KEY_F9:return Window::KEY_F9;
  case GLFW_KEY_F10:return Window::KEY_F10;
  case GLFW_KEY_F11:return Window::KEY_F11;
  case GLFW_KEY_F12:return Window::KEY_F12;
  case GLFW_KEY_F13:return Window::KEY_F13;
  case GLFW_KEY_F14:return Window::KEY_F14;
  case GLFW_KEY_F15:return Window::KEY_F15;
  case GLFW_KEY_F16:return Window::KEY_F16;
  case GLFW_KEY_F17:return Window::KEY_F17;
  case GLFW_KEY_F18:return Window::KEY_F18;
  case GLFW_KEY_F19:return Window::KEY_F19;
  case GLFW_KEY_F20:return Window::KEY_F20;
  case GLFW_KEY_F21:return Window::KEY_F21;
  case GLFW_KEY_F22:return Window::KEY_F22;
  case GLFW_KEY_F23:return Window::KEY_F23;
  case GLFW_KEY_F24:return Window::KEY_F24;
  case GLFW_KEY_F25:return Window::KEY_F25;
  case GLFW_KEY_KP_0:return Window::KEY_KP_0;
  case GLFW_KEY_KP_1:return Window::KEY_KP_1;
  case GLFW_KEY_KP_2:return Window::KEY_KP_2;
  case GLFW_KEY_KP_3:return Window::KEY_KP_3;
  case GLFW_KEY_KP_4:return Window::KEY_KP_4;
  case GLFW_KEY_KP_5:return Window::KEY_KP_5;
  case GLFW_KEY_KP_6:return Window::KEY_KP_6;
  case GLFW_KEY_KP_7:return Window::KEY_KP_7;
  case GLFW_KEY_KP_8:return Window::KEY_KP_8;
  case GLFW_KEY_KP_9:return Window::KEY_KP_9;
  case GLFW_KEY_KP_DECIMAL:return Window::KEY_KP_DECIMAL;
  case GLFW_KEY_KP_DIVIDE:return Window::KEY_KP_DIVIDE;
  case GLFW_KEY_KP_MULTIPLY:return Window::KEY_KP_MULTIPLY;
  case GLFW_KEY_KP_SUBTRACT:return Window::KEY_KP_SUBTRACT;
  case GLFW_KEY_KP_ADD:return Window::KEY_KP_ADD;
  case GLFW_KEY_KP_ENTER:return Window::KEY_KP_ENTER;
  case GLFW_KEY_KP_EQUAL:return Window::KEY_KP_EQUAL;
  case GLFW_KEY_LEFT_SHIFT:return Window::KEY_LEFT_SHIFT;
  case GLFW_KEY_LEFT_CONTROL:return Window::KEY_LEFT_CONTROL;
  case GLFW_KEY_LEFT_ALT:return Window::KEY_LEFT_ALT;
  case GLFW_KEY_LEFT_SUPER:return Window::KEY_LEFT_SUPER;
  case GLFW_KEY_RIGHT_SHIFT:return Window::KEY_RIGHT_SHIFT;
  case GLFW_KEY_RIGHT_CONTROL:return Window::KEY_RIGHT_CONTROL;
  case GLFW_KEY_RIGHT_ALT:return Window::KEY_RIGHT_ALT;
  case GLFW_KEY_RIGHT_SUPER:return Window::KEY_RIGHT_SUPER;
  case GLFW_KEY_MENU:return Window::KEY_MENU;
  default:return -1;
  }
}
static inline int translateModifier(int modifier) {
  switch (modifier) {
  case GLFW_MOD_SHIFT:return Window::SHIFT;
  case GLFW_MOD_CONTROL:return Window::CONTROL;
  case GLFW_MOD_ALT:return Window::ALT;
  case GLFW_MOD_SUPER:return Window::SUPER;
  case GLFW_MOD_CAPS_LOCK:return Window::CAPS_LOCK;
  case GLFW_MOD_NUM_LOCK:return Window::NUM_LOCK;
  default:return 0;
  }
}

void keyboard_callback(GLFWwindow *window, int glfw_key, int, int glfw_action,
					   int glfw_mods) {
  auto state = translateState(glfw_action);
  auto key = translateKey(glfw_key);
  auto modifier = translateModifier(glfw_mods);

  if (state!=Window::KEY_STATE_REPEAT)
	_lastState = state;

  // int type, int state, int key, bool repeat
  mainWindow->keyEvent(state==Window::KEY_STATE_REPEAT ? _lastState : state,
					   key, modifier, state==Window::KEY_STATE_REPEAT);
}

Window::Window(int width, int height, int flags) {
  mainWindow = std::shared_ptr<Window>(this);

  m_width = width;
  m_height = height;

  initGL();
}

Window::~Window() {}

void Window::keyEvent(int state, int key, int modifier, bool repeat) {}

std::shared_ptr<Window> Window::instance() { return mainWindow; }

void Window::mouseWheelEvent(float xoffset, float yoffset) {
  if (camera)
	camera->processMouseScroll(yoffset);
}

void Window::mouseMoveEvent(float xoffset, float yoffset) {
  if (camera)
	camera->processMouseMovement(xoffset, yoffset);
}

void Window::mouseButtonEvent(int, int, int) {}

void Window::setCamera(std::shared_ptr<Camera> _camera) {
  camera = _camera;
  camera->updateCameraVectors();
  camera->setPerspective(45.f, m_width, m_height, 0.01f, 300.f);
}

void Window::quit() { m_quit = true; }

bool Window::isRuning() { return not m_quit; }

void Window::clear() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool Window::render() { return true; }

void Window::swap() { glfwSwapBuffers(m_window); }

void Window::process() {
  calculateDuration();
  if (camera)
	camera->updateShader();
  processEvents();
}

void Window::calculateDuration() {
  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto duration =
	  std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

  if (m_time==0)
	m_deltaTime = 0;
  else
	m_deltaTime = (duration.count() - m_time)/1000.f;
  m_time = duration.count();

  if (m_verbose)
	std::cout << "FPS => " << 1.f/m_deltaTime << std::endl;
}

void Window::processEvents() {
  glfwPollEvents();

  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE)==GLFW_PRESS)
	m_quit = true;

  float cameraSpeed = static_cast<float>(2.5*m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_W)==GLFW_PRESS)
	camera->processKeyboard(FORWARD, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_S)==GLFW_PRESS)
	camera->processKeyboard(BACKWARD, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_A)==GLFW_PRESS)
	camera->processKeyboard(LEFT, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_D)==GLFW_PRESS)
	camera->processKeyboard(RIGHT, m_deltaTime);
  if (glfwGetKey(m_window, GLFW_KEY_SPACE)==GLFW_PRESS)
	camera->processKeyboard(JUMP, m_deltaTime);
}

bool Window::isOpen() { return true; }

bool Window::isVisible() {
  // Is the window open and visible, ie. not minimized?
  if (!m_window)
	return false;

  return false;
}

bool Window::isFocused() {
  return glfwGetWindowAttrib(m_window, GLFW_FOCUSED)==GLFW_TRUE;
}

bool Window::isMinimized() {
  return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED)==GLFW_TRUE;
}

bool Window::isMaximized() {
  return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED)==GLFW_TRUE;
}

unsigned int Window::getWindowId() { return m_windowId; }

void Window::setFocus() {
  glfwSetWindowAttrib(m_window, GLFW_FOCUSED, GLFW_TRUE);
}

void Window::minimize() { glfwIconifyWindow(m_window); }

void Window::maximize() { glfwMaximizeWindow(m_window); }

void Window::restore() { glfwRestoreWindow(m_window); }

void Window::hide() { glfwHideWindow(m_window); }

void Window::show() { glfwShowWindow(m_window); }

void Window::close() { delete this; }

bool Window::isFullscreen() { return m_fullscreen; }

void Window::setFullscreen(const bool fullscreen) {
  if (fullscreen)
	glfwSetWindowMonitor(m_window, glfwGetPrimaryMonitor(), 0, 0, m_width,
						 m_height, GLFW_DONT_CARE);
  else
	glfwSetWindowMonitor(m_window, NULL, 0, 0, m_width, m_height,
						 GLFW_DONT_CARE);
}

bool Window::setSize(int width, int height) {
  m_width = width;
  m_height = height;

  glfwSetWindowSize(m_window, width, height);
  glfwGetFramebufferSize(m_window, &m_width, &m_height);
  glViewport(0, 0, m_width, m_height);

  if (camera)
	camera->setPerspective(45.0f, (float)m_width, (float)m_height, 0.125f,
						   512.0f);

  return true;
}

bool Window::initGL() {
  glfwSetErrorCallback([](int, const char *desc) {
	std::cerr << desc << "\n";
	std::exit(EXIT_FAILURE);
  });
  if (!glfwInit()) {
	std::cout << "GLFW failed to Initialize!" << std::endl;
	return -1;
  }

  GLfloat vertices[] = {-0.5f, -0.5f*float(sqrt(3))/3, 0.0f,
	  0.5f, -0.5f*float(sqrt(3))/3, 0.0f,
	  0.0f, 0.5f*float(sqrt(3))*2/3, 0.0f};

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
  glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
  glfwMakeContextCurrent(m_window);
  glfwSetCursorPosCallback(m_window, mouse_callback);
  glfwSetKeyCallback(m_window, keyboard_callback);
  glfwSetScrollCallback(m_window, scroll_callback);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwGetFramebufferSize(m_window, &m_width, &m_height);
  glViewport(0, 0, m_width, m_height);

  bool success = true;
  GLenum error = GL_NO_ERROR;

  if (m_verbose)
	std::cout << glGetString(GL_VERSION) << std::endl;

  // Initialize clear color
  glClearColor(0.f, 0.f, 0.f, 1.f);
  // Check for error
  error = glGetError();
  if (error!=GL_NO_ERROR) {
	std::cout << "Error: glClearColor" << std::endl;
	success = false;
  }
  glViewport(0, 0, m_width, m_height);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return success;
}
