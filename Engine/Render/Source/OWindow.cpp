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

void OWindow::keyEvent(int, int, int, bool)
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

void OWindow::quit() {
    m_quit = true;
}

bool OWindow::isRuning() {
    return not m_quit;
}

bool OWindow::render() {
    return true;
}

void OWindow::process() {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_QUIT:
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = evt.key;
                if (mainWindow)
                    mainWindow->keyEvent(key.type,key.state,key.keysym.scancode,key.repeat);
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


bool OWindow::isOpen()
{
    return m_handle;
}

bool OWindow::isVisible()
{
    // Is the window open and visible, ie. not minimized?
    if(!m_handle)
        return false;
    
    unsigned int flags = SDL_GetWindowFlags( m_handle );
    if( flags & SDL_WINDOW_SHOWN)
        return true;

    return false;
}

bool OWindow::isFocused()
{
    unsigned int flags = SDL_GetWindowFlags( m_handle );
    if( flags & SDL_WINDOW_INPUT_FOCUS || flags & SDL_WINDOW_INPUT_GRABBED || flags & SDL_WINDOW_MOUSE_FOCUS )
        return true;

    return false;
}

bool OWindow::isMinimized()
{
    unsigned int flags = SDL_GetWindowFlags( m_handle );
    if( flags & SDL_WINDOW_MINIMIZED)
        return true;

    return false;
}

bool OWindow::isMaximized()
{
    unsigned int flags = SDL_GetWindowFlags( m_handle );
    if( flags & SDL_WINDOW_MAXIMIZED)
        return true;

    return false;
}

unsigned int OWindow::getWindowId()
{
    return m_windowId;
}

void OWindow::setFocus()
{
    SDL_SetWindowInputFocus(m_handle);
}

void OWindow::minimize()
{
    SDL_MinimizeWindow( m_handle );
}

void OWindow::maximize()
{
    SDL_MaximizeWindow( m_handle );
}

void OWindow::restore()
{
    SDL_RestoreWindow( m_handle );
}

void OWindow::hide()
{
    SDL_HideWindow( m_handle );
}

void OWindow::show()
{
    SDL_ShowWindow( m_handle );
}

void OWindow::close()
{
    delete this;
}

bool OWindow::isFullscreen()
{
    unsigned int flags = SDL_GetWindowFlags( m_handle );
    if( flags & SDL_WINDOW_FULLSCREEN || flags & SDL_WINDOW_FULLSCREEN_DESKTOP )
        return true;

    return false;
}


void OWindow::setFullscreen(const bool fullscreen)
{
    if(fullscreen)
        SDL_SetWindowFullscreen( m_handle, SDL_WINDOW_FULLSCREEN);
    else
        SDL_SetWindowFullscreen( m_handle, 0);
}

bool OWindow::setSize( int width, int height )
{
    m_width = width;
    m_height = height;

    SDL_SetWindowSize(m_handle, m_width, m_height);

    return true;
}
