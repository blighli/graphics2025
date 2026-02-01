#version 460
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PickingPush {
    vec4 color;
} pickingPush;

void main() {
    outColor = pickingPush.color;
}
