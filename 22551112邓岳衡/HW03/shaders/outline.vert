#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
} sceneUBO;

layout(set = 0, binding = 1) uniform PlanetUBO {
    mat4 model;
    vec4 color;
} planetUBO;

layout(push_constant) uniform OutlinePush {
    vec4 colorAndScale;
} outlinePush;

void main() {
    float scaleFactor = 1.0 + outlinePush.colorAndScale.w;
    vec3 scaledPosition = inPosition * scaleFactor;     // scale in local model space
    vec4 worldPosition = planetUBO.model * vec4(scaledPosition, 1.0);
    gl_Position = sceneUBO.proj * sceneUBO.view * worldPosition;
}
