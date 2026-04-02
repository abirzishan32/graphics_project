#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoords;

uniform int useTexture;
uniform sampler2D texture1;
uniform float objectAlpha;
uniform bool preserveLogoAspect;
uniform float logoAspect;
uniform float panelAspect;
uniform vec3 logoFrameColor;

void main()
{
    vec4 texColor = vec4(1.0);
    if (useTexture > 0) {
        vec2 sampleUV = TexCoords;
        if (preserveLogoAspect) {
            float safeLogoAspect = max(logoAspect, 0.0001);
            float safePanelAspect = max(panelAspect, 0.0001);
            float fitX = 1.0;
            float fitY = 1.0;

            if (safeLogoAspect > safePanelAspect) {
                fitY = safePanelAspect / safeLogoAspect;
            } else {
                fitX = safeLogoAspect / safePanelAspect;
            }

            vec2 centered = (TexCoords - vec2(0.5)) / vec2(fitX, fitY) + vec2(0.5);
            if (centered.x < 0.0 || centered.x > 1.0 || centered.y < 0.0 || centered.y > 1.0) {
                FragColor = vec4(Color * logoFrameColor, objectAlpha);
                return;
            }
            sampleUV = centered;
        }
        texColor = texture(texture1, sampleUV);
    }
    
    // The Color varies depending on Gouraud illumination calculated per-vertex
    FragColor = vec4(Color * texColor.rgb, objectAlpha * texColor.a);
}
