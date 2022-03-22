#include "OSystem.h"
#include "SDL.h"
#include "SDL_syswm.h"

using namespace omega::system;

int OSystem::init() {
    int res = SDL_Init(
            SDL_INIT_VIDEO |
            SDL_INIT_JOYSTICK |
            SDL_INIT_HAPTIC |
            SDL_INIT_GAMECONTROLLER |
            SDL_INIT_EVENTS |
            SDL_INIT_NOPARACHUTE);

    if(res<0)
        return res;

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

    return res;
}

