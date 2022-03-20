#pragma once

#include "OKeyCodes.h"

struct SDL_Window;
union SDL_Event;

namespace omega {
    namespace render {
        class OWindow {
        public:
            enum KeyState { KEY_STATE_UP = 0 , KEY_STATE_DOWN = 1};
            enum KeyType {
                KEY_DOWN        = 0x300, /**< Key pressed */
                KEY_UP,                  /**< Key released */
                TEXTEDITING,            /**< Keyboard text editing (composition) */
                TEXTINPUT,              /**< Keyboard text input */
                KEYMAPCHANGED
            };

        public:
            OWindow(int width = 800, int height = 600, int flags = 0);

            ~OWindow();

            unsigned int getWindowId();

            virtual bool isOpen();
            virtual bool isVisible();
            virtual bool isFocused();
            virtual bool isMinimized();
            virtual bool isMaximized();

            virtual void minimize();
            virtual void maximize();
            virtual void hide();
            virtual void show();
            virtual void close();
            virtual void restore();
            virtual void setFocus();
            virtual bool isFullscreen();
            virtual void setFullscreen(const bool fullscreen);

            bool isRuning();
            static void process();
            virtual bool render();
            bool setSize( int width, int height );

        protected:
            virtual void keyEvent(int, int, int, bool);
            virtual void mouseWheelEvent(int, int);
            virtual void mouseMoveEvent(int, int, int);
            virtual void mouseButtonEvent(int, int, int);

            void quit();
        private:
            SDL_Window *m_handle = nullptr;
            unsigned int m_windowId = -1;
            int m_width = 800;
            int m_height = 600;
            bool m_quit = false;
        };
    }
}


