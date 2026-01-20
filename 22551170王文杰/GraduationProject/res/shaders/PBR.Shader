#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;

out vec3 Normal;
out vec2 TexCoords;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	Normal = mat3(model) * aNormal;
	TexCoords = aTex;
	WorldPos = vec3(model * vec4(aPos, 1.0));

	gl_Position = projection * view * vec4(WorldPos, 1.0f);
}


#shader fragment
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal;
in vec3 WorldPos;

struct Material
{
	vec3 color;
	float roughness;
	float metallic;
	float ao;
};

struct Light
{
	vec3 pos;
	vec3 color;
};

uniform Material material;
uniform Light light;
uniform vec3 camPos;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{
	vec3 N = normalize(Normal);
	vec3 V = normalize(camPos - WorldPos);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, material.color, material.metallic);

	vec3 Lo = vec3(0.0);

	// calculate radiance
	vec3 L = normalize(light.pos - WorldPos);
	vec3 H = normalize(V + L);
	float distance = length(light.pos - WorldPos);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = light.color * attenuation;

	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, material.roughness);
	float G = GeometrySmith(N, V, L, material.roughness);
	vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - material.metallic;

	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	float NdotL = max(dot(N, L), 0.0);
	Lo += (kd * material.color / PI + specular) * radiance * NdotL;

	vec3 ambient = vec3(0.03) * material.color * material.ao;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(color, 1.0f);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}