#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vPosWS;
out vec3 vNrmWS;

void main(){
    vec4 wpos = uModel * vec4(aPos, 1.0);
    vPosWS = wpos.xyz;
    vNrmWS = mat3(transpose(inverse(uModel))) * aNrm;
    gl_Position = uProj * uView * wpos;
}
