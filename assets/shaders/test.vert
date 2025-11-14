#version 430 core

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;
uniform float u_time;

uniform sampler2D u_heightmap;
uniform float u_amp = 0.5f;
uniform float u_speed = 0.3f;
uniform vec2  u_texel;

uniform float u_height_mult = 0.7;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec3 vs_worldpos;
out vec3 vs_normal;

void main(void)
{
    // base world position
    vec4 world_pos = model_matrix * vec4(position, 1.0);

    vec2 flow = vec2(0.01f, 0.05f) * u_speed;

    // sample center and neighbors for derivative
    vec2 uv = texcoord * 0.9 + 0.05;
    uv += flow * u_time;

    float hC = texture(u_heightmap, uv).r;
    float hL = texture(u_heightmap, uv + vec2(-u_texel.x, 0.0)).r;
    float hR = texture(u_heightmap, uv + vec2( u_texel.x, 0.0)).r;
    float hD = texture(u_heightmap, uv + vec2(0.0, -u_texel.y)).r;
    float hU = texture(u_heightmap, uv + vec2(0.0,  u_texel.y)).r;

    // Reduce height map effect and mix with neighbors
    hC = mix(hC, (hL + hR + hD + hU) * u_height_mult, 0.1);

    // Add wide sine wave
    float sineWave = sin(world_pos.x * 0.1 + u_time * 0.1) * 0.2;
    // Combine height map and sine wave
    float finalHeight = hC * u_amp * 0.7 + sineWave * 0.3; // Adjust blending ratio

    // Apply height to world position
    world_pos.y += finalHeight;

    float dHdx = (hR - hL) * 0.5 / u_texel.x;
    float dHdy = (hU - hD) * 0.5 / u_texel.y;
    vec3 n_model = normalize(vec3(-dHdx * (u_amp), 1.0, -dHdy * (u_amp)));

    // Transform normal to world space using inverse-transpose of model
    mat3 normal_matrix = transpose(inverse(mat3(model_matrix)));
    vs_normal = normalize(normal_matrix * n_model);

    vs_worldpos = world_pos.xyz;

    gl_Position = proj_matrix * view_matrix * world_pos;
}