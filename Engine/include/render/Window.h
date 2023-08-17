#pragma once

#include <render/Camera.h>
#include <system/Global.h>
#include <render/KeyCodes.h>

class GLFWwindow;

namespace omega {
namespace render {
class OMEGA_EXPORT OWindow {
 public:
  enum KeyState { KEY_STATE_UP = 0, KEY_STATE_DOWN = 1 };
  enum KeyType {
    KEY_DOWN = 0x300, /**< Key pressed */
    KEY_UP,           /**< Key released */
    TEXTEDITING,      /**< Keyboard text editing (composition) */
    TEXTINPUT,        /**< Keyboard text input */
    KEYMAPCHANGED
  };

 public:
  OWindow(int width = 1024, int height = 768, int flags = 0);

  ~OWindow();

  unsigned int getWindowId();

  virtual bool isOpen();
  virtual bool isVisible();
  virtual bool isFocused();
  virtual bool isMinimized();
  virtual bool isMaximized();

  virtual void clear();
  virtual void swap();

  virtual void minimize();
  virtual void maximize();
  virtual void hide();
  virtual void show();
  virtual void close();
  virtual void restore();
  virtual void setFocus();
  virtual bool isFullscreen();
  virtual void setFullscreen(const bool fullscreen);
  virtual void process();

  bool isRuning();
  virtual bool render();
  bool setSize(int width, int height);

  void setCamera(std::shared_ptr<Camera>);

  static std::shared_ptr<OWindow> instance();

  inline bool isKeyPressed(omega::system::KeyCode kc) { return keys[kc]; }

  virtual void keyEvent(int, int, int, bool);
  virtual void mouseWheelEvent(float, float);
  virtual void mouseMoveEvent(float, float);
  virtual void mouseButtonEvent(int, int, int);

 protected:
  bool initGL();
  void quit();

 private:
  void processEvents();
  void calculateDuration();

 private:
  GLFWwindow* m_window;
  unsigned int m_windowId = -1;
  bool _firstMouse = true;
  float _lastX = 0;
  float _lastY = 0;

  std::shared_ptr<Camera> camera;

 protected:
  bool keys[omega::system::SDL_NUM_SCANCODES];
  int m_width = 800;
  int m_height = 600;
  bool m_quit = false;
  long long m_time = 0;
  float m_deltaTime = 0.f;
  bool m_fullscreen = false;
};
}  // namespace render
}  // namespace omega
