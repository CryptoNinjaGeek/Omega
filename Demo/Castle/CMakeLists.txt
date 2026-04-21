cmake_minimum_required(VERSION 3.21)

project(Castle
        VERSION 1.0
        DESCRIPTION "Castle labyrinth POC for the Omega portal engine"
        LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Castle
        main.cpp
        CastleWindow.cpp)

# OUTPUT_NAME is "CastleDemo" (not "Castle") because macOS's default APFS
# is case-insensitive and the user's castle texture assets live under
# `bin/castle/` — so `bin/Castle` would collide with that directory and
# the linker fails with errno=21 (EISDIR). Keeping the CMake target name
# `Castle` for consistency with Demo/Portal; only the on-disk binary name
# differs.
set_target_properties(Castle PROPERTIES
        OUTPUT_NAME "CastleDemo"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER "Demo"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

include_directories(Castle PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Castle oEngine)
target_compile_definitions(Castle PRIVATE IMPORT_ENGINE_LIB)
target_compile_options(Castle PRIVATE -Wno-deprecated-declarations)

# Ship the scene JSON + shaders the Portal renderer needs alongside the
# binary. Castle textures already live at bin/castle/textures/ on disk
# (shipped by the user) and are discovered via FileSystem's disk overlay
# once CastleWindow registers exeDir as a search root — no copy step
# needed for them.
add_custom_command(TARGET Castle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/castle_labyrinth.json
        ${CMAKE_SOURCE_DIR}/bin/castle_labyrinth.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.vs
        ${CMAKE_SOURCE_DIR}/bin/portal.vs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.fs
        ${CMAKE_SOURCE_DIR}/bin/portal.fs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/core.vs
        ${CMAKE_SOURCE_DIR}/bin/core.vs
    COMMENT "Copying Castle scene JSON and portal/core shaders to bin/"
    VERBATIM
)
