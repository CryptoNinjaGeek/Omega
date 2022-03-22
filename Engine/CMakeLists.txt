cmake_minimum_required(VERSION 3.21)
project(oEngine)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_library(oEngine SHARED
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
        Render/Source/OSystem.cpp
        Render/Header/OSystem.h
        Render/Header/OKeyCodes.h
        Render/Source/OCamera.cpp
        Render/Header/OCamera.h Render/Header/OShader.h)

include_directories(oEngine PUBLIC
        Geometry/Header
        Render/Header
        ${SDL2_INCLUDE_DIR}
        )

set(INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIR}
        ${GLUT_INCLUDE_DIR}
)

target_link_libraries(oEngine
        ${SDL2_LIBRARY}
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARIES}
)
