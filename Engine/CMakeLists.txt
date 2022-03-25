cmake_minimum_required(VERSION 3.21)
project(oEngine)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(oEngine SHARED
        System/Header/OGlobal.h
        Geometry/Source/OQuaternion.cpp
        Geometry/Header/OQuaternion.h      
        Geometry/Source/OPoint2.cpp
        Geometry/Source/OPoint3.cpp
        Geometry/Source/OPlane.cpp
        Geometry/Source/OMatrix.cpp
        Geometry/Header/OPoint2.h
        Geometry/Header/OPoint3.h
        Geometry/Header/OMath.h
        Geometry/Header/OPlane.h
        Geometry/Header/OMatrix.h
        Geometry/Header/OPoint4.h
        Geometry/Header/OBox.h
        Geometry/Header/OSphere.h
        Geometry/Header/OBoxBase.h
        Render/Source/OWindow.cpp
        Render/Header/OWindow.h
        System/Source/OSystem.cpp
        System/Header/OSystem.h
        Render/Header/OKeyCodes.h
        Render/Source/OCamera.cpp
        Render/Header/OCamera.h 
        Render/Header/OShader.h
        Render/Source/OShader.cpp
)

include_directories(oEngine PUBLIC
        ../3rdParty/include  
        System/Header
        Geometry/Header
        Render/Header
        ${SDL2_INCLUDE_DIR}
        )

link_directories(oEngine ../3rdParty/lib/x64)

set(INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIR}
        ${GLUT_INCLUDE_DIR}
)

target_link_libraries(oEngine
        ${SDL2_LIBRARY}
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARIES}
        ${OmegaEngine_SOURCE_DIR}/3rdParty/lib/x64/glew32.lib
        ${OmegaEngine_SOURCE_DIR}/3rdParty/lib/x64/glm_static.lib
)

target_compile_definitions(oEngine PRIVATE BUILD_ENGINE_LIB)
