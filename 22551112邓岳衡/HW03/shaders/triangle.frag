#version 460
#pragma shader_stage(fragment)


layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, inTexCoord);
}