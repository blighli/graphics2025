#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragUV;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
} sceneUBO;

layout(set = 0, binding = 1) uniform PlanetUBO {
    mat4 model;
    vec4 color; // default color if no texture
} planetUBO;

void main() {
    mat4 model = planetUBO.model;
    gl_Position = sceneUBO.proj * sceneUBO.view * model * vec4(inPosition, 1.0);
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragColor = planetUBO.color.xyz;
    fragUV = inTexCoord;
}
