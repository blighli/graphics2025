#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
// layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// proj和view的变换在几何着色器里进行

void main()
{
	FragPos = vec3(model * vec4(aPos, 1.0));
	// gl_Position = projection * view * model * vec4(aPos, 1.0);
	gl_Position = model * vec4(aPos, 1.0);
	TexCoord = aTexCoord;
}