#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::~Shader() {
    if (id) glDeleteProgram(id);
}

std::string Shader::readTextFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "❌ Cannot open shader file: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

unsigned int Shader::compile(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, 4096, nullptr, log);
        std::cerr << "❌ Shader compile error:\n" << log << "\n";
    }
    return s;
}

unsigned int Shader::link(unsigned int vs, unsigned int fs) {
    unsigned int p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, 4096, nullptr, log);
        std::cerr << "❌ Program link error:\n" << log << "\n";
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

bool Shader::loadFromFiles(const std::string& vsPath, const std::string& fsPath) {
    std::string vsCode = readTextFile(vsPath);
    std::string fsCode = readTextFile(fsPath);
    if (vsCode.empty() || fsCode.empty()) return false;

    unsigned int vs = compile(GL_VERTEX_SHADER, vsCode.c_str());
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fsCode.c_str());
    id = link(vs, fs);
    return id != 0;
}

void Shader::use() const {
    glUseProgram(id);
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, &m[0][0]);
}
void Shader::setVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(id, name), 1, &v[0]);
}
void Shader::setFloat(const char* name, float f) const {
    glUniform1f(glGetUniformLocation(id, name), f);
}
void Shader::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(id, name), v);
}
