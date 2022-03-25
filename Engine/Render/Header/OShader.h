#pragma once

#include<iostream>
#include<fstream>
#include<string>

#include "OPoint2.h"
#include "OPoint3.h"
#include "OPoint4.h"
#include "OMatrix.h"
#include "OGlobal.h"

#include "glm/mat4x4.hpp"

namespace omega {
    namespace render {
        using namespace  omega::geometry;

        class OMEGA_EXPORT  OShader {
        private:
            unsigned int id;
            const int versionMajor;
            const int versionMinor;

            //Private functions
            std::string loadShaderSource(std::string fileName);
            unsigned int loadShader(unsigned int type, std::string fileName);
            void linkProgram(unsigned int vertexShader, unsigned int geometryShader, unsigned int fragmentShader);

        public:

            OShader(const int versionMajor, const int versionMinor,
                std::string vertexFile, std::string fragmentFile, std::string geometryFile = {});

            ~OShader();

            //Set uniform functions
            void use();
            void unuse();
            void set1i(int value, std::string name);
            void set1f(float value, std::string name);
            void setVec2f(OPoint2<float> value, std::string name);
            void setVec3f(OPoint3<float> value, std::string name);
            void setVec4f(OPoint4<float> value, std::string name);
            void setMat4fv(OMatrix<float> value, std::string name, bool transpose = false);
            void setMat4fv(glm::mat4 value, std::string name, bool transpose = false);
        };

        typedef std::shared_ptr<OShader> OShaderPtr;
    }
}
