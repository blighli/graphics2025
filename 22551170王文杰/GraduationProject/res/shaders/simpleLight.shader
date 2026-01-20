#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}


#shader fragment
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
	FragColor = vec4(color, 1.0f);
}