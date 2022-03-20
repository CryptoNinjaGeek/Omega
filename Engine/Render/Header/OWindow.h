#pragma once

struct SDL_Window;
union SDL_Event;

namespace omega {
    namespace render {
        class OWindow {
        public:
            OWindow(int width = 800, int height = 600 , int flags = 0);
            ~OWindow();

        static int init();
        static void run();

        virtual void keyEvent(int,int,int,int);
        virtual void mouseWheelEvent(int,int);
        virtual void mouseMoveEvent(int,int,int);
        virtual void mouseButtonEvent(int,int,int);

        private:
            SDL_Window *m_handle = nullptr;
            unsigned int m_windowId = -1;
        };
    }
}


