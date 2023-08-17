cmake_minimum_required(VERSION 3.21)
project(oEngine)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

add_library(oEngine SHARED
        src/geometry/Quaternion.cpp
        src/geometry/Point2.cpp
        src/geometry/Point3.cpp
        src/geometry/Plane.cpp
        src/geometry/Matrix.cpp
        src/render/Window.cpp
        src/system/System.cpp
        src/render/Shader.cpp
        src/render/Texture.cpp
        src/other/glad.c

        include/system/Global.h
        include/geometry/Quaternion.h
        include/geometry/Point2.h
        include/geometry/Point3.h
        include/geometry/Math.h
        include/geometry/Plane.h
        include/geometry/Matrix.h
        include/geometry/Point4.h
        include/geometry/Box.h
        include/geometry/Sphere.h
        include/geometry/BoxBase.h
        include/render/Window.h
        include/system/System.h
        include/render/KeyCodes.h
        include/render/Camera.h
        include/render/Shader.h
        include/render/Texture.h
        include/geometry/Object.h
        src/geometry/Object.cpp
        include/utils/ObjectGenerator.h
        src/utils/ObjectGenerator.cpp
        include/render/Material.h
        include/interface/Entity.h
        include/interface/Light.h
        include/render/DirectionalLight.h
        include/render/SpotLight.h
        include/render/PointLight.h
        src/render/PointLight.cpp
        src/render/DirectionalLight.cpp
        src/render/SpotLight.cpp
        include/system/FileSystem.h
        src/system/FileSystem.cpp
        include/system/ByteArray.h
        include/render/CubeTexture.h
        src/render/CubeTexture.cpp
        src/geometry/SkyBox.cpp
        include/geometry/SkyBox.h
)

include_directories(oEngine PUBLIC include)

set(INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIR}
        ${GLUT_INCLUDE_DIR}
)

target_link_libraries(oEngine
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARIES}
        glm::glm
        glfw
        zip
)

target_compile_definitions(oEngine PRIVATE BUILD_ENGINE_LIB GL_SILENCE_DEPRECATION)
