#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform float uTime;

// 简单 hash + value noise
float hash(vec2 p){
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float noise(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1,0));
    float c = hash(i + vec2(0,1));
    float d = hash(i + vec2(1,1));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a,b,u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

float fbm(vec2 p){
    float v = 0.0;
    float amp = 0.5;
    for(int i=0;i<5;i++){
        v += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return v;
}

void main(){
    // 天空渐变（上蓝下浅）
    vec3 top = vec3(0.20, 0.35, 0.65);
    vec3 bottom = vec3(0.75, 0.85, 1.00);
    float h = smoothstep(0.0, 1.0, vUV.y);
    vec3 sky = mix(bottom, top, h);

    // 云：用 fbm 做噪声，随时间漂移
    vec2 p = vUV * vec2(3.0, 2.0);
    p += vec2(uTime * 0.02, uTime * 0.01);

    float n = fbm(p);
    float clouds = smoothstep(0.55, 0.75, n);  // 云密度阈值
    vec3 cloudCol = vec3(1.0);

    // 云只在上半部分更明显
    clouds *= smoothstep(0.2, 0.95, vUV.y);

    vec3 col = mix(sky, cloudCol, clouds * 0.75);

    FragColor = vec4(col, 1.0);
}
