#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// 输出到片段着色器
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

// 统一变量
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    // 法线矩阵：消除缩放对法线的影响
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    // 最终顶点位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}