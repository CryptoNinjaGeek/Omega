cmake_minimum_required(VERSION 3.21)
project(OmegaEngine)

include("FetchContent")

message(STATUS "${Green} FreeGlut Should Be Downloaded ${ColourReset}")

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_DEFAULT_CMP0063 "NEW")

FetchContent_Declare(
        reactphysics3d
        GIT_REPOSITORY https://github.com/DanielChappuis/reactphysics3d.git
        GIT_TAG v0.10.0
)

FetchContent_Declare(
        libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG v1.10.1
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
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        libpng
        GIT_REPOSITORY https://github.com/glennrp/libpng.git
        GIT_TAG v1.6.43
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.10.0
        GIT_SHALLOW TRUE
)

message(STATUS "${Green} GLFW Should Be Downloaded ${ColourReset}")
message(STATUS "${Green} GLM Should Be Downloaded ${ColourReset}")

FetchContent_MakeAvailable(reactphysics3d)
FetchContent_MakeAvailable(libzip)
FetchContent_MakeAvailable(audeo)
FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(libpng)
FetchContent_MakeAvailable(spdlog)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(Engine)
add_subdirectory(Demo)
