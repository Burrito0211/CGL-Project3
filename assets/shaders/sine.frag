#version 430 core

in V_OUT
{
    vec3 worldPos;
    vec3 worldNormal;
    vec2 texCoord;
    float waveHeight;
} f_in;

out vec4 f_color;

uniform vec3 u_cameraPos;
uniform vec3 u_lightPos = vec3(15.0, 20.0, 10.0);
uniform vec3 u_lightColor = vec3(1.0);
uniform vec3 u_deepColor = vec3(0.02, 0.26, 0.45);
uniform vec3 u_shallowColor = vec3(0.12, 0.55, 0.68);
uniform float u_ambientStrength = 0.25;
uniform float u_diffuseStrength = 1.0;
uniform float u_specularStrength = 0.35;
uniform float u_shininess = 48.0;

void main()
{
    vec3 N = normalize(f_in.worldNormal);
    vec3 L = normalize(u_lightPos - f_in.worldPos);
    vec3 V = normalize(u_cameraPos - f_in.worldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), u_shininess);

    float heightFactor = clamp(f_in.waveHeight * 0.5 + 0.5, 0.0, 1.0);
    vec3 baseColor = mix(u_deepColor, u_shallowColor, heightFactor);

    vec3 ambient = u_ambientStrength * baseColor;
    vec3 diffuse = diff * u_diffuseStrength * baseColor * u_lightColor;
    vec3 specular = spec * u_specularStrength * u_lightColor;

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 fresnelColor = mix(baseColor, vec3(0.7, 0.9, 1.0), clamp(fresnel * 0.6, 0.0, 1.0));

    vec3 color = ambient + diffuse + specular;
    color = mix(color, fresnelColor, 0.2);
    color = clamp(color, 0.0, 1.0);

    f_color = vec4(color, 1.0);
}