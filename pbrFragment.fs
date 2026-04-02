#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
// Color computed at the vertex stage (received by interpolation)
in vec3 VertexColor;

// Camera position for specular calculation
uniform vec3 viewPos;

// Material properties
uniform vec3 objectColor;
uniform float ambientStrength;   // Ka
uniform float diffuseStrength;   // Kd
uniform float specularStrength;  // Ks
uniform float shininess;         // n (exponent)
uniform float objectAlpha;

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

// Point Lights (ceiling fixtures + entrance bars)
#define MAX_POINT_LIGHTS 32
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

// Volumetric Light Scattering - window positions
#define MAX_WINDOWS 8
uniform vec3 windowPositions[MAX_WINDOWS];
uniform int numWindows;
uniform vec3 sunDirection;

// ============================================================
// Texture Uniforms
// ============================================================
uniform sampler2D texture1;    // brick-wall
uniform sampler2D texture2;    // container

// useTexture modes:
//   0 = procedural only (no image texture)
//   1 = simple texture (image texture replaces object color, no blend)
//   2 = vertex-blended (mix image texture with color computed in VERTEX shader)
//   3 = fragment-blended (mix image texture with object color, factor computed per-fragment)
//   4 = panel blend (texture1 base + texture2 overlay alpha)
//   5 = direct digit display (texture1)
//   6 = world-space tiled texture1 (repeat, no stretch)
uniform int useTexture;

// Procedural texture type (used when useTexture == 0)
// 0=concrete, 1=painted line, 2=car paint, 3=metal, 5=mirror, 6=elevator cable, 7=ribbed step, 8=floor tile helper
uniform int textureType;


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
float calculateLightShaft(vec3 worldPos, vec3 windowPos, vec3 lightDir) {
    vec3 toFragment = worldPos - windowPos;
    float projLen = dot(toFragment, lightDir);
    if (projLen < 0.0 || projLen > 15.0) return 0.0;
    vec3 closestPointOnRay = windowPos + lightDir * projLen;
    float perpDist = length(worldPos - closestPointOnRay);
    float beamWidth = 0.8 + projLen * 0.15;
    float intensity = exp(-perpDist * perpDist / (beamWidth * beamWidth));
    float distFalloff = max(1.0 - projLen / 18.0, 0.0);
    float dustNoise = noise3D(worldPos * 2.0);
    dustNoise = 0.7 + dustNoise * 0.6;
    return intensity * distFalloff * dustNoise;
}

// Calculate total volumetric scattering from all windows
vec3 calculateVolumetricScattering(vec3 worldPos) {
    vec3 scattering = vec3(0.0);
    vec3 sunlightColor = vec3(1.0, 0.75, 0.4);
    vec3 rayDir = normalize(sunDirection);
    for (int i = 0; i < numWindows && i < MAX_WINDOWS; i++) {
        float shaftIntensity = calculateLightShaft(worldPos, windowPositions[i], rayDir);
        scattering += sunlightColor * shaftIntensity;
    }
    return scattering;
}

// ============================================================
// Phong Illumination Model
// ============================================================

// Directional Light: rays are parallel (sun/infinite source).
// We negate light.direction to get the direction towards the light source.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(-light.direction);
    vec3 ambient  = light.ambient * ambientStrength * baseColor;
    float NdotL   = max(dot(normal, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * diffuseStrength * NdotL * baseColor;
    vec3 reflectDir = reflect(-lightDir, normal);
    float RdotV   = max(dot(reflectDir, viewDir), 0.0);
    float spec    = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    return ambient + diffuse + specular;
}

// Point Light: illuminates in all directions, fading with the inverse-square law.
// Attenuation formula: f_att = 1.0 / (K_c + K_l*d + K_q*d^2)
// Applied to ambient, diffuse and specular equally.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    // Direction from fragment towards the light source
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Distance-based attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Ambient
    vec3 ambient  = light.ambient * ambientStrength * baseColor;
    
    // Diffuse
    float NdotL   = max(dot(normal, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * diffuseStrength * NdotL * baseColor;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float RdotV   = max(dot(reflectDir, viewDir), 0.0);
    float spec    = pow(RdotV, shininess);
    vec3 specular = light.specular * specularStrength * spec;
    
    // Apply attenuation to all components
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

// ============================================================
// Procedural Texturing (used when useTexture == 0)
// ============================================================
vec3 proceduralConcrete(vec2 uv, vec3 baseColor) {
    float n1 = fract(sin(dot(floor(uv * 50.0),  vec2(12.9898, 78.233))) * 43758.5453);
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

vec3 proceduralMirror(vec3 baseColor, vec3 viewDir, vec3 normal) {
    vec3 refl = reflect(-viewDir, normal);
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
    float skyFactor = clamp(refl.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 envLow = vec3(0.14, 0.16, 0.19);
    vec3 envHigh = vec3(0.72, 0.80, 0.92);
    vec3 envColor = mix(envLow, envHigh, skyFactor);
    float metallicTint = 0.25;
    return mix(baseColor, envColor, 0.75 + 0.2 * fresnel) + metallicTint * fresnel;
}

vec3 proceduralElevatorCable(vec3 worldPos, vec3 baseColor) {
    float helixA = sin(worldPos.y * 240.0 + worldPos.x * 180.0);
    float helixB = sin(worldPos.y * 240.0 - worldPos.z * 180.0);
    float strand = (helixA * 0.5 + helixB * 0.5) * 0.5 + 0.5;
    float micro = fract(sin(dot(floor(worldPos * 600.0), vec3(17.3, 29.7, 47.1))) * 43758.5453);

    float darkBand = smoothstep(0.25, 0.75, strand);
    vec3 shaded = mix(baseColor * 0.70, baseColor * 1.18, darkBand);
    shaded += vec3((micro - 0.5) * 0.08);
    return clamp(shaded, 0.0, 1.0);
}

vec3 proceduralRibbedStep(vec3 worldPos, vec3 baseColor) {
    float rib = abs(sin(worldPos.x * 140.0));
    float groove = smoothstep(0.0, 0.35, rib);
    vec3 dark = baseColor * 0.55;
    vec3 light = baseColor * 1.05;
    vec3 stepped = mix(dark, light, groove);

    float grain = fract(sin(dot(floor(worldPos * 420.0), vec3(9.3, 17.1, 31.7))) * 43758.5453);
    stepped += vec3((grain - 0.5) * 0.03);
    return clamp(stepped, 0.0, 1.0);
}

// ============================================================
// Main
// ============================================================
void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec2 uv      = TexCoords; 
    vec2 sampleUV = uv;
    float finalAlpha = objectAlpha;
    bool isDigitDisplay = false;
    
    // --------------------------------------------------------
    // Determine base color according to texture mode
    // --------------------------------------------------------
    vec3 baseColor;

    if (useTexture == 1) {
        // State 1: direct texture sample.
        // For first-floor slab (textureType 8), use world-space UVs to force repetition without stretching.
        if (textureType == 8) {
            sampleUV = FragPos.xz * 0.20;
        }
        vec3 imgColor = texture(texture1, sampleUV).rgb;
        if (textureType == 4) baseColor = mix(objectColor, imgColor, 0.5);
        else baseColor = imgColor;
        
    } else if (useTexture == 2) {
        // State 2: container
        vec3 imgColor = texture(texture2, sampleUV).rgb;
        if (textureType == 4) baseColor = mix(objectColor, imgColor, 0.5);
        else baseColor = imgColor;
        
    } else if (useTexture == 4) {
        // Elevator panel compensation: rotate button texture 180 degrees.
        if (textureType == 4) {
            sampleUV = vec2(1.0 - uv.x, 1.0 - uv.y);
        }
        vec3 baseTex = texture(texture1, sampleUV).rgb;
        vec4 overlay = texture(texture2, sampleUV);
        baseColor = mix(baseTex, overlay.rgb, overlay.a);

    } else if (useTexture == 5) {
        // Seven-segment display: rotate UV to upright orientation and treat as emissive.
        if (textureType == 6) {
            sampleUV = vec2(uv.y, 1.0 - uv.x);
        }
        vec4 digitTex = texture(texture1, sampleUV);
        baseColor = digitTex.rgb;
        finalAlpha *= max(digitTex.a, 0.75);
        isDigitDisplay = true;

    } else {
        // State 0: STARTING STATE (Procedural / base objectColor)
        baseColor = objectColor;
        vec2 procUV = FragPos.xz * 0.1;
        if      (textureType == 0) baseColor = proceduralConcrete(procUV, objectColor);
        else if (textureType == 1) baseColor = proceduralPaintedLine(procUV, objectColor);
        else if (textureType == 2) baseColor = proceduralCarPaint(procUV, objectColor, viewDir, norm);
        else if (textureType == 3) baseColor = proceduralMetal(procUV, objectColor);
        else if (textureType == 5) baseColor = proceduralMirror(objectColor, viewDir, norm);
        else if (textureType == 6) baseColor = proceduralElevatorCable(FragPos, objectColor);
        else if (textureType == 7) baseColor = proceduralRibbedStep(FragPos, objectColor);
    }
    
    // --------------------------------------------------------
    // Accumulate Phong lighting from all sources (Multiple Lights)
    // --------------------------------------------------------
    vec3 result = vec3(0.0);

    if (isDigitDisplay) {
        // Keep LED display readable during movement regardless of scene lighting.
        result = clamp(baseColor * 1.6, 0.0, 1.0);
        result = pow(result, vec3(1.0 / 2.2));
        FragColor = vec4(result, finalAlpha);
        return;
    }
    
    // 1. Directional light (parallel rays from the sun through windows)
    if (useDirLight) {
        result += CalcDirLight(dirLight, norm, viewDir, baseColor);
    }
    
    // 2. Point lights (ceiling fixtures + entrance bar lights)
    //    Each contributes ambient + diffuse + specular with distance attenuation
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor);
    }
    
    // 3. Global ambient fallback
    vec3 globalAmbient = baseColor * (lightsOn ? 0.1 : 0.02);
    result += globalAmbient;
    
    // 4. Volumetric scattering (god rays inside parking lot)
    float volumetricStrength = lightsOn ? 0.08 : 0.25;
    if (FragPos.x > -1.0 && FragPos.x < 62.0 &&
        FragPos.z > -1.0 && FragPos.z < 42.0 &&
        FragPos.y > 0.0  && FragPos.y < 4.0) {
        vec3 volumetric = calculateVolumetricScattering(FragPos);
        result += volumetric * volumetricStrength;
    }
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    result = min(result, vec3(1.0));
    
    FragColor = vec4(result, finalAlpha);
}
