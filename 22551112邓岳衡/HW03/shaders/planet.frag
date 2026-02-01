#version 460
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

void main() {
    // Simple directional lighting
    // Assume light comes from the sun (0,0,0) - but for simplicity let's just use a fixed light direction for now
    // or better, if it's the sun, it should be unlit (bright), and planets lit by it.
    // For this simple scene, let's just make everything lit by a "global" light to see shapes.
    
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.1);
    vec3 sampledColor = texture(albedoTexture, fragUV).rgb;
    vec3 baseColor = sampledColor;
    outColor = vec4(baseColor * diff, 1.0);
}
