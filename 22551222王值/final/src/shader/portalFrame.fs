#version 330 core
out vec4 frag_color;
in vec2 tex_coord;

uniform sampler2D color_texture;

void main()
{
    frag_color = texture(color_texture, tex_coord);
}