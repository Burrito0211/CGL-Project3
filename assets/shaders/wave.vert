#version 330 core

layout (location = 0) in vec3 aPos;        // 原始頂點位置 (x, y, z) — y = 0
layout (location = 1) in vec3 aNormal;     // 原始法線 (0, 1, 0)
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 vNormal;
out vec3 vColor;
out vec2 vTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

struct Wave {
    vec2 direction;
    float amplitude;
    float frequency;
    float speed;
};

uniform int waveCount;
uniform Wave waves[8]; // 最多 8 個波

void main()
{
    float y = 0.0;
    vec2 posXZ = aPos.xz;

    // 計算高度
    for (int i = 0; i < waveCount; ++i) {
        float phase = dot(waves[i].direction, posXZ) * waves[i].frequency + waves[i].speed * time;
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