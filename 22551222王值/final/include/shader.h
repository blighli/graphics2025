#ifndef SHADER_H
#define SHADER_H

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
    public:
    unsigned int ID;
    // enum Type {VS, TCS, TES, GS, FS};
    enum Type {VS, GS, FS};

    Shader(const char* vsPath, const char* gsPath, const char* fsPath) {
        // 1. 从文件路径中获取顶点/片段着色器
        unsigned int vs, gs, fs;
        std::string vsCode, tcsCode, tesCode, gsCode, fsCode;
        vsCode = readShader(vsPath);
        gsCode = readShader(gsPath);
        fsCode = readShader(fsPath);
        vs = compileShader(vsCode, Type::VS);
        gs = compileShader(gsCode, Type::GS);
        fs = compileShader(fsCode, Type::FS);
        ID = glCreateProgram();
        glAttachShader(ID, vs);
        glAttachShader(ID, gs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);
        // 打印连接错误（如果有的话）
        int success;
        char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        if(!success) {
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << infoLog << std::endl;
        } else {
            std::cout << "SHADER::PROGRAM::LINKING_SUCCESS" << infoLog << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(gs);
        glDeleteShader(fs);   
    }
    Shader(const char* vsPath, const char* fsPath) {
        // 1. 从文件路径中获取顶点/片段着色器
        unsigned int vs, fs;
        std::string vsCode, tcsCode, tesCode, gsCode, fsCode;
        vsCode = readShader(vsPath);
        fsCode = readShader(fsPath);
        vs = compileShader(vsCode, Type::VS);
        fs = compileShader(fsCode, Type::FS);
        ID = glCreateProgram();
        glAttachShader(ID, vs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);
        // 打印连接错误（如果有的话）
        int success;
        char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        if(!success) {
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << infoLog << std::endl;
        } else {
            std::cout << "SHADER::PROGRAM::LINKING_SUCCESS" << infoLog << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);   
    }
    // 编译着色器，用完记得删除
    static unsigned int compileShader(std::string code, Type type) {
        unsigned int shader;
        switch (type)
        {
        case Type::VS:
            shader = glCreateShader(GL_VERTEX_SHADER);
            break;
        // case Type::TCS:
        //     shader = glCreateShader(GL_TESS_CONTROL_SHADER);
        //     break;
        // case Type::TES:
        //     shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        //     break;
        case Type::GS:
            shader = glCreateShader(GL_GEOMETRY_SHADER);
            break;
        case Type::FS:
            shader = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        }
        const char* shaderCode = code.c_str();
        glShaderSource(shader, 1, &shaderCode, NULL);
        glCompileShader(shader);
        // 打印编译错误（如果有的话）
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if(!success) {
            std::cout << type << ": ERROR::SHADER::COMPILATION_FAILED" << infoLog << std::endl;
        } else {
            std::cout << type << ": SHADER::COMPILATION_SUCCESS" << infoLog << std::endl;
        }
        return shader;
    }
    // 读取文件
    static std::string readShader(const char* shaderPath) {
    std::string prefix = "..\\src\\shader\\";
    std::ifstream file;
    std::stringstream shaderStream;
    std::string code;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        std::string fullPath = prefix + shaderPath;
        file.open(fullPath.c_str());
        shaderStream << file.rdbuf();
        file.close();
        code = shaderStream.str();
    }
    catch(std::ifstream::failure &e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    // std::cout << shaderPath << std::endl;
    // std::cout << "code----------------------------\n" << code << std::endl;
    return code;
}
    // 使用/激活程序
    void use() {
        glUseProgram(ID);
    }
    // uniform工具函数
    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }
    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) const
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};

#endif