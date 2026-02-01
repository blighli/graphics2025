#version 460
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform OutlinePush {
    vec4 colorAndScale;
} outlinePush;

void main() {
    outColor = vec4(outlinePush.colorAndScale.rgb, 1.0);
}
