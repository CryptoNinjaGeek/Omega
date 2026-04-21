cmake_minimum_required(VERSION 3.21)

project(Outdoor
        VERSION 1.0
        DESCRIPTION "Outdoor terrain demo for omega engine (Phase 1 MVP)"
        LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Outdoor main.cpp)

set_target_properties(Outdoor PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER "Demo"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

include_directories(Outdoor PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Outdoor oEngine)
target_compile_definitions(Outdoor PRIVATE IMPORT_ENGINE_LIB)
target_compile_options(Outdoor PRIVATE -Wno-deprecated-declarations)

# Copy the terrain shaders to bin/. The shipped resources.zip may not contain
# them yet; FileSystem's `:/` lookup falls back to the disk overlay in the
# executable directory, so these copies cover the demo without needing to
# repackage the zip.
add_custom_command(TARGET Outdoor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/terrain.vs
        ${CMAKE_SOURCE_DIR}/bin/terrain.vs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/terrain.fs
        ${CMAKE_SOURCE_DIR}/bin/terrain.fs
    # prop.fs pairs with the core.vs already shipped in resources.zip. The
    # fragment stage ships via the disk-overlay fallback because repacking
    # the zip per-build would be overkill for a single shader.
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/prop.fs
        ${CMAKE_SOURCE_DIR}/bin/prop.fs
    COMMENT "Copying terrain + prop shaders to bin directory"
    VERBATIM
)
