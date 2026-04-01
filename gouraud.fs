#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoords;

uniform int useTexture;
uniform sampler2D texture1;
uniform float objectAlpha;

void main()
{
    vec4 texColor = vec4(1.0);
    if (useTexture > 0) {
        texColor = texture(texture1, TexCoords);
    }
    
    // The Color varies depending on Gouraud illumination calculated per-vertex
    FragColor = vec4(Color * texColor.rgb, objectAlpha * texColor.a);
}
