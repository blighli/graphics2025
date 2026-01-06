#version 330 core
in vec3 vPosWS;
out vec4 FragColor;

uniform int uShowGrid;

float lineMask(float coord, float scale, float thickness)
{
    float x = abs(fract(coord * scale) - 0.5);
    // 越接近 0.5 越靠近线中心
    float m = smoothstep(0.5 - thickness, 0.5, x);
    return 1.0 - m;
}

void main()
{
    // 基础地面颜色（稍亮一点）
    vec3 base = vec3(0.18, 0.18, 0.20);

    if (uShowGrid == 0) {
        FragColor = vec4(base, 1.0);
        return;
    }

    // 小格：每 1.0 一个格；大格：每 5.0 一个格
    float minor = lineMask(vPosWS.x, 1.0, 0.03) + lineMask(vPosWS.z, 1.0, 0.03);
    float major = lineMask(vPosWS.x, 0.2, 0.06) + lineMask(vPosWS.z, 0.2, 0.06);

    minor = clamp(minor, 0.0, 1.0);
    major = clamp(major, 0.0, 1.0);

    // 线颜色
    vec3 col = base;
    col = mix(col, vec3(0.30, 0.30, 0.33), minor * 0.55);
    col = mix(col, vec3(0.55, 0.55, 0.60), major * 0.75);

    // 让地面中心稍亮、远处稍暗（更有空间感）
    float dist = length(vPosWS.xz);
    float vignette = clamp(dist / 35.0, 0.0, 1.0);
    col *= mix(1.15, 0.70, vignette);

    FragColor = vec4(col, 1.0);
}
