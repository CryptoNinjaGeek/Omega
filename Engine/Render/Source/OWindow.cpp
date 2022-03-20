#include "OWindow.h"
#include "SDL.h"
#include "SDL_syswm.h"

using namespace omega::render;

OWindow *mainWindow = nullptr;

OWindow::OWindow(int width, int height, int flags) {
    mainWindow = this;

    m_handle = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
    m_windowId = SDL_GetWindowID(m_handle);
}

OWindow::~OWindow() {
    SDL_DestroyWindow(m_handle);
}

void OWindow::keyEvent(int, int, int, int)
{
}

void OWindow::mouseWheelEvent(int,int)
{

}


void OWindow::mouseMoveEvent(int,int,int)
{

}


void OWindow::mouseButtonEvent(int,int,int)
{

}



int OWindow::init() {
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS |
                    SDL_INIT_NOPARACHUTE);
}


void OWindow::run() {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_QUIT: {
                return;
            }

            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = evt.key;
                if (mainWindow)
                    mainWindow->keyEvent(key.type,key.state,key.keysym.sym,key.repeat);
                break;
            }

            case SDL_MOUSEWHEEL: {
                auto wheel = evt.wheel;
                if (mainWindow)
                    mainWindow->mouseWheelEvent(wheel.type,wheel.direction);
                break;
            }

            case SDL_MOUSEMOTION: {
                auto motion = evt.motion;
                if (mainWindow)
                    mainWindow->mouseMoveEvent(motion.type,motion.x,motion.y);
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                auto button = evt.button;
                if (mainWindow)
                    mainWindow->mouseButtonEvent(button.button,button.state,button.type);
                break;
            }
        }
    }
}