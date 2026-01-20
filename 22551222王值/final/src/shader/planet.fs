#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 geomFragPos;

uniform vec3 viewPos;
uniform vec3 lightPosA;
uniform vec3 lightPosB;
uniform vec3 lightColorA;
uniform vec3 lightColorB;
uniform vec3 objectColor;

void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * (lightColorA + lightColorB);

    vec3 norm = normalize(Normal);
    vec3 lightDirA = normalize(lightPosA - geomFragPos);
    vec3 lightDirB = normalize(lightPosB - geomFragPos);

    // diffuse
    float diffA = max(dot(norm, lightDirA), 0.0);
    float diffB = max(dot(norm, lightDirB), 0.0);
    vec3 diffuseA = diffA * lightColorA;
    vec3 diffuseB = diffB * lightColorB;

    // specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - geomFragPos);
    vec3 reflectDirA = reflect(-lightDirA, norm);
    vec3 reflectDirB = reflect(-lightDirB, norm);
    float specA = pow(max(dot(viewDir, reflectDirA), 0.0), 32);
    float specB = pow(max(dot(viewDir, reflectDirB), 0.0), 32);
    vec3 specularA = specularStrength * specA * lightColorA;
    vec3 specularB = specularStrength * specB * lightColorB;

    vec3 result = (ambient + diffuseA + diffuseB + specularA + specularB) * objectColor;
    FragColor = vec4(result, 1.0);
}
