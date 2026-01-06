#version 330 core
in vec3 vPosWS;
in vec3 vNrmWS;
out vec4 FragColor;

struct DirLight {
    vec3 dir;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 pos;
    vec3 color;
    float intensity;
    float a;
    float b;
    float c;
};

uniform vec3 uCamPos;
uniform DirLight uDir;

uniform int uPointCount;
uniform PointLight uPoints[8];

uniform vec3 uAlbedo;
uniform float uShininess;

// ✅ 新增：曝光控制
uniform float uExposure;

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor, float intensity){
    float ndl = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);

    vec3 diffuse = ndl * uAlbedo;
    vec3 specular = spec * vec3(0.35); // 稍微压一下高光
    return (diffuse + specular) * lightColor * intensity;
}

void main(){
    vec3 N = normalize(vNrmWS);
    vec3 V = normalize(uCamPos - vPosWS);

    // ambient（别太高，否则会灰）
    vec3 color = vec3(0.03) * uAlbedo;

    // Directional
    vec3 Ld = normalize(-uDir.dir);
    color += blinnPhong(N, V, Ld, uDir.color, uDir.intensity);

    // Points
    for(int i=0;i<uPointCount;i++){
        vec3 toL = uPoints[i].pos - vPosWS;
        float dist = length(toL);
        vec3 L = toL / max(dist, 1e-4);

        float att = 1.0 / (uPoints[i].a + uPoints[i].b * dist + uPoints[i].c * dist * dist);
        color += blinnPhong(N, V, L, uPoints[i].color, uPoints[i].intensity * att);
    }

    // ✅ Tone Mapping + Gamma
    // filmic-ish: 1 - exp(-x * exposure)
    color = vec3(1.0) - exp(-color * uExposure);
    color = pow(max(color, vec3(0.0)), vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
