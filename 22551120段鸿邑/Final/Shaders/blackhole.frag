#version 330 core
out vec4 FragColor;
in vec2 uv;

uniform vec3 camPos;
uniform mat3 camRot;
uniform float spin;

const float Rs = 1.0;
const float STEP = 0.02;
const int MAX_STEPS = 720;

// ================= HDR 星空 =================
vec3 starfield(vec3 d) {
    float n = fract(sin(dot(d.xy, vec2(12.9898,78.233))) * 43758.5453);
    float stars = smoothstep(0.997, 1.0, n);
    return vec3(stars) * 5.5;
}

// ================= 体积吸积盘 =================
float diskVolume(vec3 p) {
    float r = length(p.xz);
    float h = abs(p.y);
    if (r < 1.8 || r > 7.0) return 0.0;

    float thickness = exp(-h * 4.5);
    float radial = smoothstep(7.0, 1.8, r);
    return thickness * radial;
}

// ================= 电影级多普勒 =================
vec3 cinematicDoppler(float v) {
    // v ∈ [-1,1]
    // 亮度主导，色相极弱
    float intensity = 1.0 + v * 0.35;   // 明暗差
    float warmth    = v * 0.05;         // 非常轻微色温

    vec3 warmBase = vec3(1.4, 1.25, 0.9); // 暖黄白（核心）
    vec3 warmTint = vec3(1.05, 1.0, 0.95);

    vec3 col = warmBase * mix(vec3(1.0), warmTint, warmth);
    return col * intensity;
}

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= 1.6;

    vec3 dir = normalize(camRot * vec3(p, -1.9));
    vec3 pos = camPos;

    vec3 color = vec3(0.0);
    float fade = 1.0;

    for (int i = 0; i < MAX_STEPS; i++) {
        float r = length(pos);

        // ================= 超大事件视界反转区 =================
        float horizonFade = smoothstep(Rs * 0.9, Rs * 2.2, r);
        fade *= mix(0.92, 1.0, horizonFade);

        if (r < Rs * 2.2) {
            vec3 inward = normalize(pos);
            dir = normalize(mix(-inward, dir, horizonFade));

            float photon = (1.0 - horizonFade) * 6.0;
            dir += -inward * photon * STEP;
        }

        // ================= 体积吸积盘 =================
        float density = diskVolume(pos);
        if (density > 0.001) {
            vec3 diskVel = normalize(cross(vec3(0,1,0), pos));
            float v = dot(diskVel, dir);

            vec3 diskCol = cinematicDoppler(v);
            color += diskCol * density * 0.045 * fade;
        }

        // ================= 强引力透镜 =================
        float lens = Rs / (r * r);
        lens *= 1.0 + 3.8 * exp(-r);

        vec3 gravity = -normalize(pos) * lens;
        dir += gravity * STEP;

        // ================= Kerr 帧拖拽 =================
        vec3 frameDrag =
            spin * cross(normalize(pos), vec3(0,1,0)) / (r * r);
        dir += frameDrag * STEP;

        dir = normalize(dir);
        pos += dir * STEP;

        if (fade < 0.002) break;
    }

    // ================= 星空（被翻转采样） =================
    color += starfield(dir) * fade;

    FragColor = vec4(color, 1.0);
}
