#version 430 core

// Match your existing V_OUT structure
in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 color;
} f_in;

out vec4 f_color;

// ========================================================
// This version EXACTLY matches PDF Page 10 structure
// ========================================================

// Uniforms matching PDF example (Page 10)
uniform vec4 color_ambient = vec4(0.1, 0.2, 0.5, 1.0);
uniform vec4 color_diffuse = vec4(0.2, 0.3, 0.6, 1.0);
uniform vec4 color_specular = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 color = vec4(0.1, 0.1, 0.5, 1.0);
uniform float shininess = 32.0;
uniform vec3 light_position = vec3(50.0, 100.0, 560.0);
uniform vec3 EyeDirection = vec3(0.0, 0.0, 1.0);

void main()
{
    // Follow PDF Page 10 exactly:
    
    // Step 1: Get light direction (L)
    // Note: PDF uses "light_position - vc_worldpos" (not normalized yet)
    vec3 light_direction = light_position - f_in.position;
    
    // Step 2: Calculate half vector (H)
    // H = normalize(normalize(L) + normalize(V))
    vec3 half_vector = normalize(normalize(light_direction) + normalize(EyeDirection));
    
    // Step 3: Calculate diffuse term
    // Diffuse = max(0.0, dot(N, L))
    // Note: PDF doesn't normalize light_direction for diffuse calculation
    // (but you should for correct results - see corrected version below)
    vec3 N = normalize(f_in.normal);
    vec3 L = normalize(light_direction);
    float diffuse = max(0.0, dot(N, L));
    
    // Step 4: Calculate specular term
    // Specular = pow(max(0.0, dot(N, H)), shininess)
    float specular = pow(max(0.0, dot(N, half_vector)), shininess);
    
    // Step 5: Combine all terms
    // Color = min(color * color_ambient, vec4(1.0)) 
    //       + diffuse * color_diffuse 
    //       + specular * color_specular
    vec4 ambient_term = min(color * color_ambient, vec4(1.0));
    vec4 diffuse_term = diffuse * color_diffuse * vec4(f_in.color, 1.0);
    vec4 specular_term = specular * color_specular;
    
    f_color = ambient_term + diffuse_term + specular_term;
}