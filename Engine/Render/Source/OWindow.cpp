#include "OWindow.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <iostream>

using namespace omega::render;
using namespace omega::geometry;
using namespace omega::system;

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                   "}\n\0";

OWindow *mainWindow = nullptr;

OWindow::OWindow(int width, int height, int flags) {
    mainWindow = this;

    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

    m_time = duration.count();
    m_handle = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    m_windowId = SDL_GetWindowID(m_handle);
    m_context = SDL_GL_CreateContext(m_handle);
    SDL_GL_MakeCurrent(m_handle, m_context);
    SDL_GL_SetSwapInterval(1);
    std::cout << glGetString(GL_VERSION) << std::endl;
    initGL();
}

OWindow::~OWindow() {
    SDL_DestroyWindow(m_handle);
}

void OWindow::keyEvent(int type, int state, int key, bool repeat) {
    if (!camera)
        return;

    auto movement = camera->onKeys(key, m_deltaTime * 0.5f);

    if (movement.len() > 0.0f)
        camera->move(movement);
}

void OWindow::mouseWheelEvent(int, int) {
}

void OWindow::mouseMoveEvent(int type, float xoffset, float yoffset) {
/*    if(camera)
        camera->processMouseMovement(xoffset, yoffset);*/
}

void OWindow::mouseButtonEvent(int, int, int) {
}

void OWindow::setCamera(std::shared_ptr<OCamera> _camera) {
    camera = _camera;
    camera->setPerspective(45.0f, (float) m_width / (float) m_height, 0.125f, 512.0f);
/*
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera->ProjectionMatrix);
*/
}

void OWindow::quit() {
    m_quit = true;
}

bool OWindow::isRuning() {
    return not m_quit;
}

bool OWindow::render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (camera) {
/*        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(camera->ViewMatrix);

        camera->ViewMatrix.dumpMatrix();
*/    }

    glUseProgram(shaderProgram);
    glBindVertexArrayAPPLE(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 6); // set the count to 6 since we're drawing 6 vertices now (2 triangles); not 3!

    SDL_GL_SwapWindow(m_handle);
    return true;
}

void OWindow::process() {
    SDL_Event evt;

    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

    m_deltaTime = (duration.count() - m_time) / 1000.f;
    m_time = duration.count();

    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_QUIT:
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = evt.key;
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
                _mouseMoveEvent(motion.type, motion.x, motion.y);
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

//    glViewport(0, 0, m_width, m_height);
/*
    if (camera) {
        camera->setPerspective(45.0f, (float) m_width / (float) m_height, 0.125f, 512.0f);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(camera->ProjectionMatrix);
    }
*/
    return true;
}


bool OWindow::initGL() {
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    bool success = true;
    GLenum error = GL_NO_ERROR;

    std::cout << glGetString(GL_VERSION) << std::endl;

    //Initialize Projection Matrix
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();

    //Check for error
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "Error: glLoadIdentity" << std::endl;
        success = false;
    }

    //Initialize Modelview Matrix
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();

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

//    glEnable(GL_DEPTH_TEST);
    //    glEnable(GL_CULL_FACE);

    demo();

    return success;
}

void OWindow::_mouseMoveEvent(int type, int xposIn, int yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (_firstMouse) {
        _lastX = xpos;
        _lastY = ypos;
        _firstMouse = false;
    }

    float xoffset = xpos - _lastX;
    float yoffset = _lastY - ypos; // reversed since y-coordinates go from bottom to top

    _lastX = xpos;
    _lastY = ypos;

    mouseMoveEvent(type, xoffset, yoffset);
}


void OWindow::demo()
{

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // add a new set of vertices to form a second triangle (a total of 6 vertices); the vertex attribute configuration remains the same (still one 3-float position vector per vertex)
    float vertices[] = {
            // first triangle
            -0.9f, -0.5f, 0.0f,  // left
            -0.0f, -0.5f, 0.0f,  // right
            -0.45f, 0.5f, 0.0f,  // top
            // second triangle
            0.0f, -0.5f, 0.0f,  // left
            0.9f, -0.5f, 0.0f,  // right
            0.45f, 0.5f, 0.0f   // top
    };

    glGenVertexArraysAPPLE(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArrayAPPLE(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArrayAPPLE(0);

}