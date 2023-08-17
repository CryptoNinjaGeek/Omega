cmake_minimum_required(VERSION 3.21)
project(OmegaEngine)

include("FetchContent")

message(STATUS "${Green} FreeGlut Should Be Downloaded ${ColourReset}")

FetchContent_Declare(
        libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG v1.9.2
)

FetchContent_Declare(
        audeo
        GIT_REPOSITORY https://github.com/NotAPenguin0/audeo.git
)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)
FetchContent_Declare(
        libpng
        GIT_REPOSITORY https://github.com/glennrp/libpng.git
        GIT_TAG v1.6.39
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.10.0
        GIT_SHALLOW TRUE
)

FetchContent_GetProperties(glfw)
if (NOT glfw_POPULATED)
    FetchContent_Populate(glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif ()
message(STATUS "${Green} GLFW Should Be Downloaded ${ColourReset}")

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
)

FetchContent_GetProperties(glm)
if (NOT glm_POPULATED)
    FetchContent_Populate(glm)
    set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
    add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
endif ()
message(STATUS "${Green} GLM Should Be Downloaded ${ColourReset}")

FetchContent_Declare(
        stb_image
        GIT_REPOSITORY https://github.com/jschmidt42/stb-cmake.git
)

FetchContent_GetProperties(stb_image)

if (NOT stb_image_POPULATED)
    FetchContent_Populate(stb_image)
endif ()

FetchContent_MakeAvailable(libzip)
FetchContent_MakeAvailable(audeo)
FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(libpng)
FetchContent_MakeAvailable(spdlog)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(Engine)
add_subdirectory(Demo)
