#version 430 core

in vec3 vs_worldpos;
in vec3 vs_normal;

out vec4 FragColor;

// ===== uniforms =====
uniform vec3 u_cameraPos;

// 水的基本顏色
uniform vec3 u_waterColor = vec3(0.0, 0.35, 0.45);

// 光源
uniform vec3 u_lightPos = vec3(10.0, 10.0, 10.0);
uniform vec3 u_lightColor = vec3(1.0);

// 強度參數
uniform float u_ambientStrength  = 0.25;
uniform float u_diffuseStrength  = 1.0;
uniform float u_specularStrength = 0.35;
uniform float u_shininess        = 32.0;

void main()
{
    // ========== 基本 normal ==========
    vec3 N = normalize(vs_normal);

    // ========== 光照 ==========
    vec3 L = normalize(u_lightPos - vs_worldpos);
    vec3 V = normalize(u_cameraPos - vs_worldpos);
    vec3 H = normalize(L + V);   // Half vector for Blinn-Phong

    // Ambient
    vec3 ambient = u_ambientStrength * u_waterColor;

    // Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * u_diffuseStrength * u_waterColor * u_lightColor;

    // Specular (Blinn-Phong)
    float spec = pow(max(dot(N, H), 0.0), u_shininess);
    vec3 specular = spec * u_specularStrength * u_lightColor;

    vec3 color = ambient + diffuse + specular;

    FragColor = vec4(color, 1.0);
}
