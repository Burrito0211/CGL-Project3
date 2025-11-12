#version 330 core

in vec3 vNormal;
in vec3 vColor;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 lightDir = normalize(vec3(0.2, 1.0, 0.3));
uniform vec3 lightColor = vec3(1.0);

void main()
{
    float diffuse = max(dot(normalize(vNormal), normalize(lightDir)), 0.0);
    vec3 color = vColor * (0.3 + 0.7 * diffuse);
    FragColor = vec4(color, 1.0);
}