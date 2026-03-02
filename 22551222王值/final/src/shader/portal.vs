#version 330 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_normal;

out vec2 tex_coord;
out vec4 color;
out vec3 frag_pos;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    gl_Position = proj * view * model * vec4( in_pos, 1.0 );
    
    frag_pos = vec3( model * vec4( in_pos, 1.0 ) );
    
    tex_coord = in_uv;
    
    color = in_color;

    vec4 temp_normal = transpose( inverse( model ) ) * vec4( in_normal, 1.0 );
    normal = temp_normal.xyz;
}