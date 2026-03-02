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
    float radius;
};

uniform vec3 uCamPos;
uniform DirLight uDir;
uniform int uPointCount;
uniform PointLight uPoints[8];

uniform vec3 uAlbedo;
uniform float uShininess;

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor, float intensity)
{
    float ndl = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);

    vec3 diffuse = ndl * uAlbedo;
    vec3 specular = spec * vec3(0.5);
    return (diffuse + specular) * lightColor * intensity;
}

void main()
{
    vec3 N = normalize(vNrmWS);
    vec3 V = normalize(uCamPos - vPosWS);

    vec3 color = vec3(0.06) * uAlbedo;

    vec3 Ld = normalize(-uDir.dir);
    color += blinnPhong(N, V, Ld, uDir.color, uDir.intensity);

    for(int i = 0; i < uPointCount; i++)
    {
        vec3 toL = uPoints[i].pos - vPosWS;
        float dist = length(toL);
        vec3 L = toL / max(dist, 1e-4);

        float att = clamp(1.0 - dist / uPoints[i].radius, 0.0, 1.0);
        att *= att;

        color += blinnPhong(
            N, V, L,
            uPoints[i].color,
            uPoints[i].intensity * att
        );
    }

    FragColor = vec4(color, 1.0);
}
