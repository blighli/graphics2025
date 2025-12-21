#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D reflect0;
    float shininess;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


in vec2 TexCoord;
in vec3 Normal;     // the fragment normal
in vec3 FragPos;    // the fragment position

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform Material material;

// Lights
#define NR_POINT_LIGHTS 1
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 envColor);


void main(){
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 envColor = vec3(0.5, 0.5, 0.5);
    vec3 result = vec3(0.0);

    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, envColor);

    FragColor = vec4(result, 1.0f);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 envColor){
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse0, TexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse0, TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(material.specular0, TexCoord));
    vec3 reflection =  light.specular * vec3(texture(material.reflect0, TexCoord)) * envColor;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    reflection *= attenuation;
    return (ambient + diffuse + specular + reflection);
}

