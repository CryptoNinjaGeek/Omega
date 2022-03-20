#include "OSystem.h"
#include "SDL.h"
#include "SDL_syswm.h"

using namespace omega::system;

int OSystem::init() {
    return SDL_Init(
            SDL_INIT_VIDEO |
            SDL_INIT_JOYSTICK |
            SDL_INIT_HAPTIC |
            SDL_INIT_GAMECONTROLLER |
            SDL_INIT_EVENTS |
            SDL_INIT_NOPARACHUTE);
}

