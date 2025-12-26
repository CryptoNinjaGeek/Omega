cmake_minimum_required(VERSION 3.21)

project(Portal 
		VERSION 1.0 
		DESCRIPTION "Portal demo for omega engine"
		LANGUAGES CXX )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Portal main.cpp)


set_target_properties(Portal PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		FOLDER "Demo"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

include_directories(Portal PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Portal oEngine)
target_compile_definitions(Portal PRIVATE IMPORT_ENGINE_LIB)
target_compile_options(Portal PRIVATE -Wno-deprecated-declarations)

# Copy scene JSON file and portal shaders to bin directory
# Ensure bin directory exists and copy the files
add_custom_command(TARGET Portal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/portal_scene.json
        ${CMAKE_SOURCE_DIR}/bin/portal_scene.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/tunnel_scene.json
        ${CMAKE_SOURCE_DIR}/bin/tunnel_scene.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.vs
        ${CMAKE_SOURCE_DIR}/bin/portal.vs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.fs
        ${CMAKE_SOURCE_DIR}/bin/portal.fs
    COMMENT "Copying scene JSON files and portal shaders to bin directory"
    VERBATIM
)

