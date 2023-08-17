#pragma once

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"

#if defined(WIN32)
#if defined(BUILD_ENGINE_LIB)
#define OMEGA_EXPORT __declspec(dllexport)
#elif defined(IMPORT_ENGINE_LIB)
#define OMEGA_EXPORT __declspec(dllimport)
#else
#define OMEGA_EXPORT
#endif
#else
#define OMEGA_EXPORT
#endif
