#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTex;

uniform mat4 model;
// uniform mat4 view;
// uniform mat4 projection;
out vec3 color;
out vec2 texcoord;

void main()
{
	gl_Position = model * vec4(aPos, 1.0f);
	color = aColor;
	// gl_Position = projection * view * vec4(aPos, 1.0f);
	texcoord = aTex;
}


#shader fragment
#version 330 core
out vec4 FragColor;
in vec3 color;
in vec2 texcoord;

uniform sampler2D tex[2];

void main()
{
	vec3 outColor = texture(tex[1], texcoord).rgb;
	// vec3 outColor = vec3(texcoord, 1.0);
	FragColor = vec4(color, 1.0f);
}