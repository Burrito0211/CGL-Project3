#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 u_model;
uniform float u_time;
uniform float u_amplitude = 1.5;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out V_OUT
{
    vec3 worldPos;
    vec3 worldNormal;
    vec2 texCoord;
    float waveHeight;
} v_out;

const vec2 WAVE_DIRECTION = normalize(vec2(0.8, 0.6));
const float WAVE_FREQUENCY = 4.0; // 2pi over 10 units to show a full wave across the mesh
const float WAVE_SPEED = 2.0;
const float WAVE_AMPLITUDE = 0.05;

void main()
{
    vec3 localPos = aPos;
    vec2 posXZ = localPos.xz;

    float phase = dot(WAVE_DIRECTION, posXZ) * WAVE_FREQUENCY + u_time * WAVE_SPEED;
    float sineValue = sin(phase);
    float cosineValue = cos(phase);

    float waveHeight = sineValue * WAVE_AMPLITUDE * u_amplitude;
    vec2 slope = WAVE_DIRECTION * (WAVE_FREQUENCY * WAVE_AMPLITUDE * u_amplitude * cosineValue);

    localPos.y += waveHeight;

    vec4 worldPosition = u_model * vec4(localPos, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    vec3 modelNormal = normalize(vec3(-slope.x, 1.0, -slope.y));
    vec3 worldNormal = normalize(normalMatrix * modelNormal);

    gl_Position = u_projection * u_view * worldPosition;

    v_out.worldPos = worldPosition.xyz;
    v_out.worldNormal = worldNormal;
    v_out.texCoord = aTexCoord;
    v_out.waveHeight = waveHeight;
}