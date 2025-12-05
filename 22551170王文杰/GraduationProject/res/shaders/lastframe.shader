#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;

out vec2 screenCoord;

void main()
{
	//gl_Position = projection * view * vec4(aPos, 1.0f);
	gl_Position = vec4(aPos, 1.0f);

	screenCoord = (vec2(aPos.x, aPos.y) + 1.0) / 2.0;
}


#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 screenCoord;

uniform sampler2D texture0;


void main()
{	
	FragColor = vec4(texture(texture0, screenCoord).xyz, 1);
	FragColor = pow(FragColor, vec4(1.0/2.2)); // Gamma correction)
}