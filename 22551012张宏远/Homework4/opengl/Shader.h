#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int id = 0;
    Shader() = default;
    ~Shader();

    bool loadFromFiles(const std::string& vsPath, const std::string& fsPath);

    void use() const;

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setFloat(const char* name, float f) const;
    void setInt(const char* name, int v) const;

private:
    static std::string readTextFile(const std::string& path);
    static unsigned int compile(unsigned int type, const char* src);
    static unsigned int link(unsigned int vs, unsigned int fs);
};
