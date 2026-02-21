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

// Lights toggle
uniform bool lightsOn;

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

// Volumetric Light Scattering (God Rays) - window positions
#define MAX_WINDOWS 8
uniform vec3 windowPositions[MAX_WINDOWS];
uniform int numWindows;
uniform vec3 sunDirection;

// Procedural texture type
uniform int textureType;  // 0=concrete, 1=painted line, 2=car paint, 3=metal, 4=glass


float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.543))) * 43758.5453);
}

// 3D noise for volumetric scattering
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // Smoothstep
    
    float n = hash(i) * (1.0 - f.x) * (1.0 - f.y) * (1.0 - f.z)
            + hash(i + vec3(1,0,0)) * f.x * (1.0 - f.y) * (1.0 - f.z)
            + hash(i + vec3(0,1,0)) * (1.0 - f.x) * f.y * (1.0 - f.z)
            + hash(i + vec3(1,1,0)) * f.x * f.y * (1.0 - f.z)
            + hash(i + vec3(0,0,1)) * (1.0 - f.x) * (1.0 - f.y) * f.z
            + hash(i + vec3(1,0,1)) * f.x * (1.0 - f.y) * f.z
            + hash(i + vec3(0,1,1)) * (1.0 - f.x) * f.y * f.z
            + hash(i + vec3(1,1,1)) * f.x * f.y * f.z;
    return n;
}

// Calculate light shaft intensity at a point
// This simulates Mie scattering in dusty air
float calculateLightShaft(vec3 worldPos, vec3 windowPos, vec3 lightDir) {
    // Vector from window to this fragment
    vec3 toFragment = worldPos - windowPos;
    
    // Project fragment position onto light ray direction
    float projLen = dot(toFragment, lightDir);
    
    // Only consider points in front of window (inside parking lot)
    if (projLen < 0.0 || projLen > 15.0) return 0.0;
    
    // Calculate perpendicular distance from light ray
    vec3 closestPointOnRay = windowPos + lightDir * projLen;
    float perpDist = length(worldPos - closestPointOnRay);
    
    // Light shaft width (wider near window, spreading out)
    float beamWidth = 0.8 + projLen * 0.15;
    
    // Gaussian falloff from ray center
    float intensity = exp(-perpDist * perpDist / (beamWidth * beamWidth));
    
    // Decrease intensity with distance from window
    float distFalloff = 1.0 - projLen / 18.0;
    distFalloff = max(distFalloff, 0.0);
    
    // Add dust/particle noise for realistic scattering
    float dustNoise = noise3D(worldPos * 2.0);
    dustNoise = 0.7 + dustNoise * 0.6; // Range 0.7-1.3
    
    return intensity * distFalloff * dustNoise;
}

// Calculate total volumetric scattering from all windows
vec3 calculateVolumetricScattering(vec3 worldPos) {
    vec3 scattering = vec3(0.0);
    
    // WARM GOLDEN SUNLIGHT - realistic afternoon sun color
    // More saturated orange-gold for visible warmth
    vec3 sunlightColor = vec3(1.0, 0.75, 0.4);
    
    // Sun ray direction (from window into the room)
    vec3 rayDir = normalize(sunDirection);
    
    for (int i = 0; i < numWindows && i < MAX_WINDOWS; i++) {
        float shaftIntensity = calculateLightShaft(worldPos, windowPositions[i], rayDir);
        
        // Accumulate light scattering with warm coloring
        scattering += sunlightColor * shaftIntensity;
    }
    
    return scattering;
}

// ============================================================
// Phong Illumination Model
// ============================================================



// input: normal, light direction (Surface normal vector at the fragment/pixel),
// view direction (Direction from the fragment toward the camera/viewer), base color


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) { 
    vec3 lightDir = normalize(-light.direction); //  light rays are parallel
    
    vec3 ambient = light.ambient * ambientStrength * baseColor;
    
    float NdotL = max(dot(normal, lightDir), 0.0); //  Measures how directly the light hits the surface
    vec3 diffuse = light.diffuse * diffuseStrength * NdotL * baseColor;
    
    vec3 reflectDir = reflect(-lightDir, normal);

    // Calculate specular reflection based on view direction
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    float spec = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos); // Rays diverge from a point, direction depends on fragment position.
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * ambientStrength * baseColor;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diffuseStrength * NdotL * baseColor;
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    float spec = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

// ============================================================
// Procedural Texturing
// ============================================================
vec3 proceduralConcrete(vec2 uv, vec3 baseColor) {
    float n1 = fract(sin(dot(floor(uv * 50.0), vec2(12.9898, 78.233))) * 43758.5453);
    float n2 = fract(sin(dot(floor(uv * 150.0), vec2(12.9898, 78.233))) * 43758.5453);
    float n3 = fract(sin(dot(floor(uv * 400.0), vec2(12.9898, 78.233))) * 43758.5453);
    float noise = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;
    vec3 variation = vec3(noise * 0.15 - 0.075);
    float spots = step(0.92, n2);
    variation -= vec3(spots * 0.1);
    return clamp(baseColor + variation, 0.0, 1.0);
}

vec3 proceduralPaintedLine(vec2 uv, vec3 paintColor) {
    float wear = fract(sin(dot(floor(uv * 200.0), vec2(12.9898, 78.233))) * 43758.5453);
    return paintColor * (1.0 - wear * 0.15);
}

vec3 proceduralCarPaint(vec2 uv, vec3 baseColor, vec3 viewDir, vec3 normal) {
    float flake = fract(sin(dot(floor(uv * 800.0), vec2(12.9898, 78.233))) * 43758.5453);
    flake = smoothstep(0.7, 1.0, flake) * 0.3;
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    return baseColor + vec3(flake) + vec3(fresnel * 0.1);
}

vec3 proceduralMetal(vec2 uv, vec3 baseColor) {
    float brush = fract(sin(uv.x * 500.0 + uv.y * 2.0) * 43758.5453);
    return baseColor + vec3(brush * 0.1 - 0.05);
}

// ============================================================
// Main
// ============================================================
void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Get base color with procedural texturing
    vec3 baseColor = objectColor;
    vec2 uv = FragPos.xz * 0.1;
    
    if (textureType == 0) {
        baseColor = proceduralConcrete(uv, objectColor);
    } else if (textureType == 1) {
        baseColor = proceduralPaintedLine(uv, objectColor);
    } else if (textureType == 2) {
        baseColor = proceduralCarPaint(uv, objectColor, viewDir, norm);
    } else if (textureType == 3) {
        baseColor = proceduralMetal(uv, objectColor);
    }
    
    // Accumulate lighting
    vec3 result = vec3(0.0);
    
    // Add directional light
    if (useDirLight) {
        result += CalcDirLight(dirLight, norm, viewDir, baseColor);
    }
    
    // Add point lights
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor);
    }
    
    // Minimum ambient
    vec3 globalAmbient = baseColor * (lightsOn ? 0.1 : 0.02);
    result += globalAmbient;
    

    
    float volumetricStrength = lightsOn ? 0.08 : 0.25;
    
    // Check if fragment is inside the parking lot volume
    if (FragPos.x > -1.0 && FragPos.x < 62.0 && 
        FragPos.z > -1.0 && FragPos.z < 42.0 && 
        FragPos.y > 0.0 && FragPos.y < 4.0) {
        
        vec3 volumetric = calculateVolumetricScattering(FragPos);
        result += volumetric * volumetricStrength;
    }
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    
    // Clamp to prevent over-brightness
    result = min(result, vec3(1.0));
    
    FragColor = vec4(result, 1.0);
}
