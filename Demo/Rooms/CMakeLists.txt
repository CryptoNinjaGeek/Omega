cmake_minimum_required(VERSION 3.21)

project(Rooms 
		VERSION 1.0 
		DESCRIPTION "Rooms demo for omega engine - doorway-based portals"
		LANGUAGES CXX )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Rooms main.cpp)


set_target_properties(Rooms PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		FOLDER "Demo"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

include_directories(Rooms PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Rooms oEngine)
target_compile_definitions(Rooms PRIVATE IMPORT_ENGINE_LIB)
target_compile_options(Rooms PRIVATE -Wno-deprecated-declarations)

# Copy scene JSON file and portal shaders to bin directory
# Ensure bin directory exists and copy the files
add_custom_command(TARGET Rooms POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/rooms_scene.json
        ${CMAKE_SOURCE_DIR}/bin/rooms_scene.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.vs
        ${CMAKE_SOURCE_DIR}/bin/portal.vs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Demo/Resources/shaders/portal.fs
        ${CMAKE_SOURCE_DIR}/bin/portal.fs
    COMMENT "Copying rooms_scene.json and portal shaders to bin directory"
    VERBATIM
)

