#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Camera position for specular calculation
uniform vec3 viewPos;

// Material properties
uniform vec3 objectColor;
uniform float ambientStrength;   // Ka
uniform float diffuseStrength;   // Kd
uniform float specularStrength;  // Ks
uniform float shininess;         // n (exponent)

// Directional Light (sunlight through windows)
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;
uniform bool useDirLight;

// Point Lights (ceiling fixtures)
#define MAX_POINT_LIGHTS 16
struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;

// Procedural texture type
uniform int textureType;  // 0=concrete, 1=painted line, 2=car paint, 3=metal, 4=glass

// ============================================================
// Phong Illumination Model:
// I = Ia * Ka + Id * Kd * max(N · L, 0) + Is * Ks * pow(max(R · V, 0), n)
// ============================================================

// Calculate directional light contribution
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(-light.direction);
    
    // Ambient component: Ia * Ka
    vec3 ambient = light.ambient * ambientStrength * baseColor;
    
    // Diffuse component: Id * Kd * max(N · L, 0)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diffuseStrength * NdotL * baseColor;
    
    // Specular component: Is * Ks * pow(max(R · V, 0), n)
    vec3 reflectDir = reflect(-lightDir, normal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    float spec = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    
    return ambient + diffuse + specular;
}

// Calculate point light contribution
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Attenuation based on distance
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Ambient component: Ia * Ka
    vec3 ambient = light.ambient * ambientStrength * baseColor;
    
    // Diffuse component: Id * Kd * max(N · L, 0)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diffuseStrength * NdotL * baseColor;
    
    // Specular component: Is * Ks * pow(max(R · V, 0), n)
    vec3 reflectDir = reflect(-lightDir, normal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    float spec = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    
    // Apply attenuation to all components
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

// Procedural concrete texture
vec3 proceduralConcrete(vec2 uv, vec3 baseColor) {
    // Multi-octave noise approximation for concrete grain
    float scale1 = 50.0;
    float scale2 = 150.0;
    float scale3 = 400.0;
    
    // Simple hash-based noise
    float n1 = fract(sin(dot(floor(uv * scale1), vec2(12.9898, 78.233))) * 43758.5453);
    float n2 = fract(sin(dot(floor(uv * scale2), vec2(12.9898, 78.233))) * 43758.5453);
    float n3 = fract(sin(dot(floor(uv * scale3), vec2(12.9898, 78.233))) * 43758.5453);
    
    // Combine noise at different frequencies
    float noise = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;
    
    // Add subtle color variation
    vec3 variation = vec3(noise * 0.15 - 0.075);
    
    // Add darker spots (aggregate)
    float spots = step(0.92, n2);
    variation -= vec3(spots * 0.1);
    
    return clamp(baseColor + variation, 0.0, 1.0);
}

// Procedural painted line texture
vec3 proceduralPaintedLine(vec2 uv, vec3 paintColor) {
    // Slight wear/fade variation
    float wear = fract(sin(dot(floor(uv * 200.0), vec2(12.9898, 78.233))) * 43758.5453);
    float fadeAmount = wear * 0.15;
    
    // Edge softening
    return paintColor * (1.0 - fadeAmount);
}

// Procedural car paint (metallic)
vec3 proceduralCarPaint(vec2 uv, vec3 baseColor, vec3 viewDir, vec3 normal) {
    // Metallic flake effect
    float flake = fract(sin(dot(floor(uv * 800.0), vec2(12.9898, 78.233))) * 43758.5453);
    flake = smoothstep(0.7, 1.0, flake) * 0.3;
    
    // View-dependent color shift (subtle)
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    
    return baseColor + vec3(flake) + vec3(fresnel * 0.1);
}

// Procedural metal texture
vec3 proceduralMetal(vec2 uv, vec3 baseColor) {
    // Brushed metal effect
    float brush = fract(sin(uv.x * 500.0 + uv.y * 2.0) * 43758.5453);
    brush = brush * 0.1 - 0.05;
    
    return baseColor + vec3(brush);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Get base color with procedural texturing
    vec3 baseColor = objectColor;
    vec2 uv = FragPos.xz * 0.1;  // World-space UV for tiling
    
    if (textureType == 0) {
        // Concrete
        baseColor = proceduralConcrete(uv, objectColor);
    } else if (textureType == 1) {
        // Painted line
        baseColor = proceduralPaintedLine(uv, objectColor);
    } else if (textureType == 2) {
        // Car paint
        baseColor = proceduralCarPaint(uv, objectColor, viewDir, norm);
    } else if (textureType == 3) {
        // Metal
        baseColor = proceduralMetal(uv, objectColor);
    }
    // textureType == 4: glass (uses objectColor directly)
    
    // Accumulate lighting from all sources
    vec3 result = vec3(0.0);
    
    // Add directional light (sunlight through windows)
    if (useDirLight) {
        result += CalcDirLight(dirLight, norm, viewDir, baseColor);
    }
    
    // Add point lights (ceiling fixtures)
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor);
    }
    
    // Ensure minimum ambient lighting (fill light)
    vec3 globalAmbient = baseColor * 0.08;
    result += globalAmbient;
    
    // Gamma correction for more realistic output
    result = pow(result, vec3(1.0/2.2));
    
    FragColor = vec4(result, 1.0);
}
