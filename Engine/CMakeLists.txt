cmake_minimum_required(VERSION 3.21)
project(oEngine)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

add_library(oEngine SHARED
        src/geometry/Plane.cpp
        src/render/Window.cpp
        src/system/System.cpp
        src/render/Shader.cpp
        src/render/Texture.cpp
        src/other/glad.c

        include/system/Global.h
        include/geometry/Math.h
        include/geometry/Plane.h
        include/geometry/BoundingBox.h
        include/geometry/Sphere.h
        include/geometry/BoxBase.h
        include/render/Window.h
        include/system/System.h
        include/render/KeyCodes.h

        include/render/Camera.h
        include/render/CameraFly.h
        include/render/CameraFPS.h

        include/render/Shader.h
        include/render/Texture.h
        include/geometry/Object.h
        src/geometry/Object.cpp
        include/utils/ObjectGenerator.h
        include/render/Material.h
        include/geometry/Entity.h
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
        include/geometry/Scene.h
        include/geometry/Vertex.h

        src/utils/ObjectGenerator.cpp
        src/utils/Objects/Box.cpp
        src/utils/Objects/Dome.cpp
        src/utils/Objects/Mesh.cpp
        src/utils/Objects/Plane.cpp
        src/utils/Objects/Container.cpp
        src/geometry/Scene.cpp
        include/physics/PhysicsObjectInput.h

        src/render/Camera.cpp
        src/render/CameraFly.cpp
        src/render/CameraFPS.cpp

        src/geometry/ComplexObject.cpp
        include/geometry/ComplexObject.h

        src/system/TextureManager.cpp
        include/system/TextureManager.h

        src/utils/Loader.cpp
        include/utils/Loader.h

        include/geometry/ObjectTree.h
        src/geometry/ObjectTree.cpp

        include/interface/ObjectInterface.h
        include/utils/utils.h

        include/physics/PhysicsEngine.h
        src/physics/PhysicsEngine.cpp
        src/geometry/Entity.cpp
        src/physics/PhysicsObject.cpp
        src/render/OpenGLRender.cpp
        include/render/OpenGLRender.h
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
        assimp
)

target_compile_definitions(oEngine PRIVATE BUILD_ENGINE_LIB GL_SILENCE_DEPRECATION)
