#include<iostream>
#include<fstream>
#include<string>

#include "OShader.h"

#if defined(WIN32)
#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL\gl.h>
#include <GL\glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

using namespace omega::render;
using namespace omega::geometry;


std::string OShader::loadShaderSource(std::string fileName) {
    std::string temp = "";
    std::string src = "";

    std::ifstream in_file;

    //Vertex
    in_file.open(fileName);

    if (in_file.is_open()) {
        while (std::getline(in_file, temp))
            src += temp + "\n";
    } else {
        std::cout << "ERROR::SHADER::COULD_NOT_OPEN_FILE: " << fileName << "\n";
    }

    in_file.close();
/*
    std::string versionNr =
            std::to_string(this->versionMajor) +
            std::to_string(this->versionMinor) +
            "0";

    src.replace(src.find("#version"), 12, ("#version " + versionNr));
    */
    return src;
}

GLuint OShader::loadShader(GLenum type, std::string fileName) {
    char infoLog[512];
    GLint success;

    GLuint shader = glCreateShader(type);
    std::string str_src = this->loadShaderSource(fileName);
    const GLchar *src = str_src.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COULD_NOT_COMPILE_SHADER: " << fileName << "\n";
        std::cout << infoLog << "\n";
    }

    return shader;
}

void OShader::linkProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader) {
    char infoLog[512];
    GLint success;

    this->id = glCreateProgram();

    glAttachShader(this->id, vertexShader);

    if (geometryShader)
        glAttachShader(this->id, geometryShader);

    glAttachShader(this->id, fragmentShader);

    glLinkProgram(this->id);

    glGetProgramiv(this->id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COULD_NOT_LINK_PROGRAM" << "\n";
        std::cout << infoLog << "\n";
    }

    glUseProgram(0);
}


//Constructors/Destructors
OShader::OShader(const int versionMajor, const int versionMinor,
        std::string vertexFile, std::string fragmentFile, std::string geometryFile )
        : versionMajor(versionMajor), versionMinor(versionMinor) 
{
    GLuint vertexShader = 0;
    GLuint geometryShader = 0;
    GLuint fragmentShader = 0;

    vertexShader = loadShader(GL_VERTEX_SHADER, vertexFile);

    if (!geometryFile.empty())
        geometryShader = loadShader(GL_GEOMETRY_SHADER, geometryFile);

    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentFile);

    this->linkProgram(vertexShader, geometryShader, fragmentShader);

    //End
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);
}

OShader::~OShader() {
    glDeleteProgram(this->id);
}

//Set uniform functions
void OShader::use() {
    glUseProgram(this->id);
}

void OShader::unuse() {
    glUseProgram(0);
}

void OShader::set1i(GLint value, std::string name) {
    this->use();

    glUniform1i(glGetUniformLocation(this->id, name.c_str()), value);

    this->unuse();
}

void OShader::set1f(GLfloat value, std::string name) {
    this->use();

    glUniform1f(glGetUniformLocation(this->id, name.c_str()), value);

    this->unuse();
}

void OShader::setVec2f(OPoint2<float> value, std::string name) {
    this->use();

    glUniform2fv(glGetUniformLocation(this->id, name.c_str()), 1, value);

    this->unuse();
}

void OShader::setVec3f(OPoint3<float> value, std::string name) {
    this->use();

    glUniform3fv(glGetUniformLocation(this->id, name.c_str()), 1, value);

    this->unuse();
}

void OShader::setVec4f(OPoint4<float> value, std::string name) {
    this->use();

    glUniform4fv(glGetUniformLocation(this->id, name.c_str()), 1, value);

    this->unuse();
}

void OShader::setMat4fv(OMatrix<float> value, const std::string name, bool transpose ) {
    this->use();

    glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, transpose, value);

    this->unuse();
}

void OShader::setMat4fv(glm::mat4 value, const std::string name, bool transpose) {
    this->use();

    glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, GL_FALSE, &value[0][0]);

    this->unuse();
}


