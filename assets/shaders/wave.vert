#version 430 core

// Input attributes
layout (location = 0) in vec3 pos;    
layout (location = 1) in vec3 normal;    
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec3 color;

// Output to fragment shader
out V_OUT
{
   vec3 position;      // World space position
   vec3 normal;        // World space normal
   vec2 texture_coordinate;
   vec3 color;         // Wave color based on height
} v_out;

// Uniforms
uniform mat4 u_model;

// Shared UBO with projection, view, and time
layout(std140, binding = 0) uniform common_matrices {
    mat4 u_projection;
    mat4 u_view;
    float u_time;       // Animation time
};

// Wave structure matching the PDF formula
struct Wave {
    vec2 direction;     // D_i: Wave direction vector
    float amplitude;    // A_i: Wave amplitude
    float frequency;    // w_i = 2π/L: Angular frequency
    float speed;        // Used to compute φ_i = S × w_i
};

// Define 3 waves (can be modified via uniforms if needed)
const Wave waves[3] = Wave[3](
    Wave(vec2(1.0, 0.0),   2.0, 0.314, 1.0),   // Wave 1
    Wave(vec2(0.7, 0.7),   3.0, 0.209, 0.8),   // Wave 2
    Wave(vec2(-0.6, 0.8),  1.5, 0.419, 1.2)    // Wave 3
);

const int waveCount = 3;
const float PI = 3.14159265359;

void main()
{
    vec2 posXZ = pos.xz;
    float y = 0.0;

    // ========================================================
    // POSITION CALCULATION (3% requirement)
    // Formula: H(x,y,t) = Σ(A_i × sin(D_i·(x,y) × w_i + t × φ_i))
    // ========================================================
    for (int i = 0; i < waveCount; ++i) {
        // Phase = D_i · (x,z) × w_i + t × φ_i
        // where φ_i = speed × w_i
        float phase = dot(waves[i].direction, posXZ) * waves[i].frequency 
                    + u_time * waves[i].speed * waves[i].frequency;
        
        // Accumulate wave height
        y += waves[i].amplitude * sin(phase);
    }

    // ========================================================
    // NORMAL CALCULATION (3% requirement)
    // Using finite difference method to approximate derivatives
    // Normal = normalize((-∂h/∂x, 1, -∂h/∂z))
    // ========================================================
    float eps = 0.1;
    float hxL = 0.0, hxR = 0.0, hzL = 0.0, hzR = 0.0;

    for (int i = 0; i < waveCount; ++i) {
        // Sample at x-eps
        float phaseXL = dot(waves[i].direction, posXZ + vec2(-eps, 0)) * waves[i].frequency 
                      + u_time * waves[i].speed * waves[i].frequency;
        hxL += waves[i].amplitude * sin(phaseXL);
        
        // Sample at x+eps
        float phaseXR = dot(waves[i].direction, posXZ + vec2(eps, 0)) * waves[i].frequency 
                      + u_time * waves[i].speed * waves[i].frequency;
        hxR += waves[i].amplitude * sin(phaseXR);
        
        // Sample at z-eps
        float phaseZL = dot(waves[i].direction, posXZ + vec2(0, -eps)) * waves[i].frequency 
                      + u_time * waves[i].speed * waves[i].frequency;
        hzL += waves[i].amplitude * sin(phaseZL);
        
        // Sample at z+eps
        float phaseZR = dot(waves[i].direction, posXZ + vec2(0, eps)) * waves[i].frequency 
                      + u_time * waves[i].speed * waves[i].frequency;
        hzR += waves[i].amplitude * sin(phaseZR);
    }
    
    // Compute normal from height gradients
    vec3 computedNormal = normalize(vec3(hxL - hxR, 2.0 * eps, hzL - hzR));

    // ========================================================
    // COLOR based on wave height (visual feedback)
    // ========================================================
    float green = clamp(0.5 + y * 0.2, 0.0, 1.0);
    float blue = clamp(0.6 + y * 0.3, 0.0, 1.0);
    v_out.color = vec3(0.0, green, blue);

    // ========================================================
    // OUTPUT TO FRAGMENT SHADER
    // ========================================================
    
    // Transform normal to world space
    v_out.normal = mat3(transpose(inverse(u_model))) * computedNormal;
    
    // World position for lighting calculations
    vec4 worldPos = u_model * vec4(pos.x, y, pos.z, 1.0);
    v_out.position = worldPos.xyz;
    
    // Pass texture coordinates
    v_out.texture_coordinate = texCoord;

    // ========================================================
    // FINAL VERTEX POSITION (2% - Animation requirement)
    // ========================================================
    gl_Position = u_projection * u_view * worldPos;
}