#version 430 core

layout(location = 0) in vec3 pos;    
layout(location = 1) in vec3 normal;    
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 color;

out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
    vec3 color;
} v_out;


uniform mat4 u_model;

// === Shared UBO with your C++ side ===
layout(std140, binding = 0) uniform common_matrices {
    mat4 u_projection;
    mat4 u_view;
    float u_time;
};

// === Wave definition ===
struct Wave {
    vec2 direction;
    float amplitude;
    float frequency;
    float speed;
};

// Now that you’re using GLSL 430, struct array initialization works:
const Wave waves[3] = Wave[3](
    Wave(vec2(1.0, 0.0),   2.0, 0.10, 1.0),
    Wave(vec2(0.7, 0.7),   3.0, 0.05, 0.8),
    Wave(vec2(-0.6, 0.8),  1.5, 0.07, 1.2)
);

const int waveCount = 3;

void main()
{
    vec2 posXZ = pos.xz;
    float y = 0.0;

    // === Combine wave heights ===
    for (int i = 0; i < waveCount; ++i) {
        float phase = dot(waves[i].direction, posXZ) * waves[i].frequency + waves[i].speed * u_time;
        y += waves[i].amplitude * sin(phase);
    }

    // === Approximate normal via finite difference ===
    float eps = 0.1;
    float hxL = 0.0, hxR = 0.0, hzL = 0.0, hzR = 0.0;

    for (int i = 0; i < waveCount; ++i) {
        hxL += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(-eps, 0)) * waves[i].frequency + waves[i].speed * u_time);
        hxR += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(+eps, 0)) * waves[i].frequency + waves[i].speed * u_time);
        hzL += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(0, -eps)) * waves[i].frequency + waves[i].speed * u_time);
        hzR += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(0, +eps)) * waves[i].frequency + waves[i].speed * u_time);
    }

    // === Color based on height ===
    float green = clamp(0.5 + y * 2.0, 0.0, 1.0);
    float blue  = clamp(0.6 + y * 3.0, 0.0, 1.0);
    v_out.color = vec3(0.0, green, blue);

    // Transform normal and pass through
        

    // === Final vertex position ===
    gl_Position = u_projection * u_view * u_model * vec4(pos.x, y, pos.z, 1.0);
    v_out.position = vec3(u_model * vec4(pos.x, y, pos.z, 1.0));
    v_out.normal = mat3(transpose(inverse(u_model))) * normal;
    v_out.texture_coordinate = texCoord;
    v_out.color = color;
}
