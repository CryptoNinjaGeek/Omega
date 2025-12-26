cmake_minimum_required(VERSION 3.21)

project(Basic 
		VERSION 1.0 
		DESCRIPTION "Basic omega demo"
		LANGUAGES CXX )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(Basic main.cpp)


set_target_properties(Basic PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		FOLDER "Demo"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

include_directories(Basic PUBLIC
        ${OmegaEngine_SOURCE_DIR}/3rdParty/include
        ../../Engine/include
)
target_link_libraries(Basic oEngine)
target_compile_definitions(Basic PRIVATE IMPORT_ENGINE_LIB)
target_compile_options(Basic PRIVATE -Wno-deprecated-declarations)
