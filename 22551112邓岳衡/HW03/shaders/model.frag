#version 460

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec3 fragBitangent;
layout(location = 4) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D specularTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;

// should match that in light.hpp
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
};

layout(std140, set = 2, binding = 0) uniform LightUniform {
    vec3 viewPos;
    vec3 ambientColor;
    PointLight pointLights[4];
    DirectionalLight dirLight;
    SpotLight spotLight;
} light;

layout(constant_id = 0) const int kPointLightCount = 0;

vec3 applyLighting(vec3 albedo, vec3 normal, vec3 specularColor, vec3 fragPos) {
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(light.viewPos - fragPos);
    vec3 result = light.ambientColor * albedo;

    // directional light
    vec3 dirLightDir = normalize(-light.dirLight.direction);
    float diff = max(dot(norm, dirLightDir), 0.0);
    vec3 diffuse = diff * light.dirLight.color * light.dirLight.intensity * albedo;
    vec3 halfDir = normalize(viewDir + dirLightDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
    vec3 specular = spec * light.dirLight.color * light.dirLight.intensity * specularColor;
    result += diffuse + specular;

    // point lights
    for (int i = 0; i < kPointLightCount; ++i) {
        vec3 lightVec = light.pointLights[i].position - fragPos;
        float distance = length(lightVec);
        vec3 lightDir = normalize(lightVec);
        float atten = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        float pDiff = max(dot(norm, lightDir), 0.0);
        vec3 pDiffuse = pDiff * light.pointLights[i].color * light.pointLights[i].intensity * albedo;
        vec3 pHalf = normalize(viewDir + lightDir);
        float pSpec = pow(max(dot(norm, pHalf), 0.0), 32.0);
        vec3 pSpecular = pSpec * light.pointLights[i].color * light.pointLights[i].intensity * specularColor;
        result += (pDiffuse + pSpecular) * atten;
    }

    // spot light
    vec3 spotDir = normalize(light.spotLight.position - fragPos);
    float theta = dot(spotDir, normalize(-light.spotLight.direction));
    float epsilon = max(light.spotLight.cutOff - light.spotLight.outerCutOff, 0.0001);
    float spotIntensity = clamp((theta - light.spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float sDiff = max(dot(norm, spotDir), 0.0);
    vec3 sDiffuse = sDiff * light.spotLight.color * light.spotLight.intensity * albedo;
    vec3 sHalf = normalize(viewDir + spotDir);
    float sSpec = pow(max(dot(norm, sHalf), 0.0), 32.0);
    vec3 sSpecular = sSpec * light.spotLight.color * light.spotLight.intensity * specularColor;
    result += (sDiffuse + sSpecular) * spotIntensity;

    return result;
}

void main() {
    vec3 albedo = texture(albedoTexture, fragUV).rgb;
    vec3 specularColor = texture(specularTexture, fragUV).rgb;
    vec3 normalSample = texture(normalTexture, fragUV).rgb;
    normalSample = normalSample * 2.0 - 1.0;
    mat3 tbn = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragNormal));
    vec3 normal = normalize(tbn * normalSample);
    vec3 litColor = applyLighting(albedo, normal, specularColor, fragWorldPos);
    outColor = vec4(litColor, 1.0);
}
