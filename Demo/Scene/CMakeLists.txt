cmake_minimum_required(VERSION 3.21)

project(Scene
        VERSION 1.0
        DESCRIPTION "Scene omega demo"
        LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Scene main.cpp)


set_target_properties(Scene PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER "Demo"
)

include_directories(Scene PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Scene oEngine)
target_compile_definitions(Scene PRIVATE IMPORT_ENGINE_LIB)
