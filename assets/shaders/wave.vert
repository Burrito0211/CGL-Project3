#version 330 core

layout (location = 0) in vec3 aPos;    
layout (location = 1) in vec3 aNormal;    
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 vNormal;
out vec3 vColor;
out vec2 vTexCoord;

uniform mat4 model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
    float u_time;
};


struct Wave {
    vec2 direction;
    float amplitude;
    float frequency;
    float speed;
};

 Wave waves[3] = {
    {{1.0f, 0.0f}, 2.0f, 0.10f, 1.0f},
    {{0.7f, 0.7f}, 3.0f, 0.05f, 0.8f},
    {{-0.6f, 0.8f}, 1.5f, 0.07f, 1.2f}
};

const int waveCount = 3;

void main()
{
    float y = 0.0;
    vec2 posXZ = aPos.xz;

    for(int i = 0; i < waveCount; ++i) {
        float phase = dot(waves[i].direction, posXZ) * waves[i].frequency + waves[i].speed * u_time;
        y += waves[i].amplitude * sin(phase);
    }

    // 計算法線（使用微分近似）
    float eps = 0.1;
    float hxL = 0.0, hxR = 0.0, hzL = 0.0, hzR = 0.0;
    for (int i = 0; i < waveCount; ++i) {
        hxL += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(-eps, 0)) * waves[i].frequency + waves[i].speed * time);
        hxR += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(+eps, 0)) * waves[i].frequency + waves[i].speed * time);
        hzL += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(0, -eps)) * waves[i].frequency + waves[i].speed * time);
        hzR += waves[i].amplitude * sin(dot(waves[i].direction, posXZ + vec2(0, +eps)) * waves[i].frequency + waves[i].speed * time);
    }
    vec3 normal = normalize(vec3(hxL - hxR, 2.0 * eps, hzL - hzR));

    // 顏色根據高度微調
    float green = clamp(0.5 + y * 2.0, 0.0, 1.0);
    float blue = clamp(0.6 + y * 3.0, 0.0, 1.0);
    vColor = vec3(0.0, green, blue);

    vNormal = mat3(transpose(inverse(model))) * normal;
    vTexCoord = aTexCoord;

    gl_Position = projection * view * model * vec4(aPos.x, y, aPos.z, 1.0);
}