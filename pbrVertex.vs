#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // World space position for lighting calculations
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Transform normal to world space (handles non-uniform scaling)
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Pass through texture coordinates
    TexCoords = aTexCoord;
    
    // Final clip-space position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}