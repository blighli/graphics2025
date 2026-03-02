#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragBitangent;
layout(location = 4) out vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
} sceneUBO;

layout(push_constant) uniform ModelPushConstant {
    mat4 model;
} modelPC;

void main() {
    mat4 modelMatrix = modelPC.model;
    vec4 worldPos = modelMatrix * vec4(inPosition, 1.0);
    gl_Position = sceneUBO.proj * sceneUBO.view * worldPos;
    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragTangent = normalize(normalMatrix * inTangent);
    fragBitangent = normalize(normalMatrix * inBitangent);
    fragWorldPos = worldPos.xyz;
    fragUV = inTexCoord;
}
