#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
// Color computed at the VERTEX stage (used by texture mode 2 - vertex-blended)
// Derived from the vertex's normalized local position to produce a smooth gradient
out vec3 VertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // World space position for lighting calculations
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Transform normal to world space using the Normal Matrix
    // (transpose of the inverse of the model matrix upper-left 3x3)
    // This correctly handles non-uniform scaling so normals remain perpendicular to surfaces.
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Pass through texture coordinates
    TexCoords = aTexCoord;

    // Vertex color computed at vertex stage: map position to a warm gradient palette
    // Used for texture mode 2 (blended with image texture in fragment shader)
    VertexColor = 0.5 + 0.5 * normalize(aPos);

    // Final clip-space position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}