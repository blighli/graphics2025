#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 texcoord;
out vec3 normal;
out vec3 worldPos;


void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	normal = aNormal;
	texcoord = aTex;
	worldPos = vec3(model * vec4(aPos, 1.0f));
}


#shader fragment
#version 330 core
out vec4 FragColor;
in vec3 normal;
in vec2 texcoord;
in vec3 worldPos;


uniform sampler2D material[15];     
uniform int textureID;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
	vec3 baceColor = texture(material[textureID], texcoord).rgb;
	vec3 N = normalize(normal);
	vec3 L = normalize(lightPos - worldPos);
	vec3 V = normalize(viewPos - worldPos);

	vec3 ambient = 0.2 * lightColor;

	vec3 diffuse = max(dot(N, L), 0) * lightColor;

	vec3 H = normalize(L + V);
	vec3 specular = pow(max(dot(N, H), 0), 32) * lightColor;

	vec3 outColor = (ambient + diffuse + specular) * baceColor;

	FragColor = vec4(outColor, 1.0f);
}