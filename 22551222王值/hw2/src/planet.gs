#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 FragPos[];
in vec2 TexCoord[];

out vec3 geomFragPos;
out vec2 geomTexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec3 edge1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 edge2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    Normal = normalize(-cross(edge1, edge2));
    for (int i = 0; i < 3; i++) {
        geomFragPos = FragPos[i];
        geomTexCoord = TexCoord[i];
        gl_Position = projection * view * gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}