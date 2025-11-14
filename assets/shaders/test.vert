#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;

uniform sampler2D u_heightmap;
uniform float u_amp = 0.5;
uniform float u_speed = 0.3;
uniform vec2 u_texel;
uniform float u_height_mult = 0.7;

out vec3 vs_worldpos;
out vec3 vs_normal;

vec2 wrapUV(vec2 uv)
{
	return uv - floor(uv);
}

float sampleHeight(vec2 uv)
{
	return texture(u_heightmap, wrapUV(uv)).r;
}

void main()
{
	vec4 worldPos = u_model * vec4(aPos, 1.0);
	vec2 flow = vec2(0.01, 0.05) * u_speed;
	vec2 uv = aTexCoord * 0.9 + 0.05 + flow * u_time;

	float hC = sampleHeight(uv);
	float hL = sampleHeight(uv + vec2(-u_texel.x, 0.0));
	float hR = sampleHeight(uv + vec2( u_texel.x, 0.0));
	float hD = sampleHeight(uv + vec2(0.0, -u_texel.y));
	float hU = sampleHeight(uv + vec2(0.0,  u_texel.y));
	float neighborAvg = (hL + hR + hD + hU) * 0.25;
	float heightMapHeight = mix(hC, neighborAvg * u_height_mult, 0.35);

	float sinePhase = worldPos.x * 0.1 + u_time * 0.1;
	float sineWave = sin(sinePhase) * 0.2;
	float finalHeight = heightMapHeight * u_amp * 0.7 + sineWave * 0.3;
	worldPos.y += finalHeight;

	float texelX = max(u_texel.x, 1e-4);
	float texelY = max(u_texel.y, 1e-4);
	float dHdx = (hR - hL) * 0.5 / texelX;
	float dHdy = (hU - hD) * 0.5 / texelY;
	float modelScaleX = length(u_model[0].xyz);
	float sineDerivativeX = 0.1 * cos(sinePhase) * modelScaleX * 0.3;
	vec3 nModel = normalize(vec3(-(dHdx * u_amp + sineDerivativeX), 1.0, -(dHdy * u_amp)));
	mat3 normalMatrix = transpose(inverse(mat3(u_model)));
	vs_normal = normalize(normalMatrix * nModel);
	vs_worldpos = worldPos.xyz;

	gl_Position = u_projection * u_view * worldPos;
}