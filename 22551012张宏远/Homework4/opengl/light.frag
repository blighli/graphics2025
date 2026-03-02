#version 330 core
out vec4 FragColor;

uniform vec3 uColor;     // 小球颜色（直接用点光颜色）
uniform float uBoost;    // 发光强度（让它更亮）

void main(){
    vec3 col = uColor * uBoost;
    // 简单的 tone/gamma（让颜色更舒服）
    col = col / (col + vec3(1.0));
    col = pow(max(col, vec3(0.0)), vec3(1.0/2.2));
    FragColor = vec4(col, 1.0);
}
