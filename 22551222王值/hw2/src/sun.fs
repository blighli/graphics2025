#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;
uniform vec3 lightColor;

void main()
{
    FragColor = texture(texture1, TexCoord) * vec4(lightColor, 1.0); 
}