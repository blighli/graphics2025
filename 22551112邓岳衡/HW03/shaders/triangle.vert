#version 460
#pragma shader_stage(vertex)

layout(push_constant) uniform PushConstants {
    vec2 instanceOffset[4];
} pc;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inInstanceOffset;
layout(location = 0) out vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition + pc.instanceOffset[gl_InstanceIndex], 0, 1);
    outTexCoord = inTexCoord;
}