#include "OWindow.h"
#include "SDL.h"
#include "SDL_syswm.h"

#if defined(WIN32)
#include  "GL/glew.h"
#include  "GL/wglew.h"
#include <GL\gl.h>
#include <GL\glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#include <iostream>
#include <chrono>
#include <iso646.h>

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;


std::shared_ptr<OWindow> mainWindow = nullptr;

OWindow::OWindow(int width, int height, int flags) {
    mainWindow = std::shared_ptr<OWindow>(this);

    m_width = width;
    m_height = height;
    
    memset(&keys[0], 0, sizeof(bool) * omega::system::SDL_NUM_SCANCODES);

    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

    m_time = duration.count();
    m_handle = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    m_windowId = SDL_GetWindowID(m_handle);
    m_context = SDL_GL_CreateContext(m_handle);

    SDL_GL_MakeCurrent(m_handle, m_context);
    SDL_GL_SetSwapInterval(1);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_CaptureMouse(SDL_TRUE);

    initGL();
}

OWindow::~OWindow() {
    SDL_DestroyWindow(m_handle);
}

void OWindow::keyEvent(int type, int state, int key, bool repeat) {
}


std::shared_ptr<OWindow> OWindow::instance()
{
    return mainWindow;
}

void OWindow::mouseWheelEvent(int, int) {
}

void OWindow::mouseMoveEvent( int type, int xoffset, int yoffset) {
//    if (camera)
//        camera->onMouseMove(xoffset, yoffset);
}

void OWindow::mouseButtonEvent(int, int, int) {
}

void OWindow::setCamera(std::shared_ptr<OCamera> _camera) {
    camera = _camera;
    camera->setPerspective(45.f, (float) m_width / (float) m_height, 0.01f, 300.f);
}

void OWindow::quit() {
    m_quit = true;
}

bool OWindow::isRuning() {
    return not m_quit;
}

void OWindow::clear()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool OWindow::render() 
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    return true;
}

void OWindow::swap()
{
    SDL_GL_SwapWindow(m_handle);
}

void OWindow::process() {
    calculateDuration();

    if (camera)
        camera->process(m_deltaTime);

    processEvents();
}

void OWindow::calculateDuration()
{
    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

    m_deltaTime = (duration.count() - m_time) / 1000.f;
    m_time = duration.count();

    //    std::cout << "FPS => " << 1.f / m_deltaTime << std::endl;
}

void OWindow::processEvents()
{
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
        case SDL_QUIT:
            break;
        case SDL_KEYDOWN: {
            auto key = evt.key;
            keys[key.keysym.scancode] = true;
            keyEvent(key.type, key.state, key.keysym.scancode, key.repeat);
            break;
        }
        case SDL_KEYUP: {
            auto key = evt.key;
            keys[key.keysym.scancode] = false;
            keyEvent(key.type, key.state, key.keysym.scancode, key.repeat);
            break;
        }

        case SDL_MOUSEWHEEL: {
            auto wheel = evt.wheel;
            mouseWheelEvent(wheel.type, wheel.direction);
            break;
        }

        case SDL_MOUSEMOTION: {
            auto motion = evt.motion;
            mouseMoveEvent(motion.type, motion.x, motion.y);
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            auto button = evt.button;
            mouseButtonEvent(button.button, button.state, button.type);
            break;
        }
        }
    }
}

bool OWindow::isOpen() {
    return m_handle;
}

bool OWindow::isVisible() {
    // Is the window open and visible, ie. not minimized?
    if (!m_handle)
        return false;

    unsigned int flags = SDL_GetWindowFlags(m_handle);
    if (flags & SDL_WINDOW_SHOWN)
        return true;

    return false;
}

bool OWindow::isFocused() {
    unsigned int flags = SDL_GetWindowFlags(m_handle);
    if (flags & SDL_WINDOW_INPUT_FOCUS || flags & SDL_WINDOW_INPUT_GRABBED || flags & SDL_WINDOW_MOUSE_FOCUS)
        return true;

    return false;
}

bool OWindow::isMinimized() {
    unsigned int flags = SDL_GetWindowFlags(m_handle);
    if (flags & SDL_WINDOW_MINIMIZED)
        return true;

    return false;
}

bool OWindow::isMaximized() {
    unsigned int flags = SDL_GetWindowFlags(m_handle);
    if (flags & SDL_WINDOW_MAXIMIZED)
        return true;

    return false;
}

unsigned int OWindow::getWindowId() {
    return m_windowId;
}

void OWindow::setFocus() {
    SDL_SetWindowInputFocus(m_handle);
}

void OWindow::minimize() {
    SDL_MinimizeWindow(m_handle);
}

void OWindow::maximize() {
    SDL_MaximizeWindow(m_handle);
}

void OWindow::restore() {
    SDL_RestoreWindow(m_handle);
}

void OWindow::hide() {
    SDL_HideWindow(m_handle);
}

void OWindow::show() {
    SDL_ShowWindow(m_handle);
}

void OWindow::close() {
    delete this;
}

bool OWindow::isFullscreen() {
    unsigned int flags = SDL_GetWindowFlags(m_handle);
    if (flags & SDL_WINDOW_FULLSCREEN || flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
        return true;

    return false;
}

void OWindow::setFullscreen(const bool fullscreen) {
    if (fullscreen)
        SDL_SetWindowFullscreen(m_handle, SDL_WINDOW_FULLSCREEN);
    else
        SDL_SetWindowFullscreen(m_handle, 0);
}

bool OWindow::setSize(int width, int height) {
    m_width = width;
    m_height = height;

    SDL_SetWindowSize(m_handle, m_width, m_height);

    glViewport(0, 0, m_width, m_height);

    if (camera)
        camera->setPerspective(45.0f, (float) m_width / (float) m_height, 0.125f, 512.0f);

    return true;
}

bool OWindow::initGL() {
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    GLenum err = glewInit();
    if (GLEW_OK != err)
        std::cout << glewGetErrorString(err) << std::endl;
     

    bool success = true;
    GLenum error = GL_NO_ERROR;

    std::cout << glGetString(GL_VERSION) << std::endl;

    //Initialize Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //Check for error
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "Error: glLoadIdentity" << std::endl;
        success = false;
    }

    //Initialize Modelview Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Check for error
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "Error: glLoadIdentity" << std::endl;
        success = false;
    }
    //Initialize clear color
    glClearColor(0.f, 0.f, 0.f, 1.f);
    //Check for error
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "Error: glClearColor" << std::endl;
        success = false;
    }
    glViewport(0, 0, m_width, m_height);

    glEnable(GL_DEPTH_TEST);
    //    glEnable(GL_CULL_FACE);

    demo();

    return success;
}

void OWindow::demo()
{
    float vertices[] = {
       -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
       -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

       -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
       -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
       -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    // world space positions of our cubes
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);



}