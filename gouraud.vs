#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 Color; // Interpolated color passed to fragment shader
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Material
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

// --- Custom Gouraud Lights ---
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;
uniform bool useDirLight;

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};
#define MAX_POINT_LIGHTS 4
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
       
    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;
uniform bool useSpotLight;

uniform vec3 viewPos;
 
 vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
     vec3 lightDir = normalize(-light.direction);
     // diffuse
     float diff = max(dot(normal, lightDir), 0.0);
     // specular
     vec3 reflectDir = reflect(-lightDir, normal);
     float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
     
     // Soften directional light significantly so it doesn't overpower indoor stalls
     vec3 diffuse = light.diffuse * diff * aColor * (diffuseStrength * 0.35);
     vec3 specular = light.specular * spec * vec3(1.0) * (specularStrength * 0.1); 
     return (diffuse + specular);
 }
 
 vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
     vec3 lightDir = normalize(light.position - fragPos);
     // diffuse 
     float diff = max(dot(normal, lightDir), 0.0);
     // specular
     vec3 reflectDir = reflect(-lightDir, normal);
     float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
     // attenuation
     float dist = length(light.position - fragPos);
     float attenuation = 1.0 / (light.constant + light.linear * dist + (light.quadratic * 0.2) * (dist * dist));
     
     vec3 diffuse = light.diffuse * diff * aColor * diffuseStrength * attenuation;
     vec3 specular = light.specular * spec * vec3(1.0) * (specularStrength * 0.2) * attenuation;
     return (diffuse + specular);
 }
 
 vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
     vec3 lightDir = normalize(light.position - fragPos);
     // diffuse 
     float diff = max(dot(normal, lightDir), 0.0);
     // specular
     vec3 reflectDir = reflect(-lightDir, normal);
     float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
     // attenuation
     float dist = length(light.position - fragPos);
     float attenuation = 1.0 / (light.constant + light.linear * dist + (light.quadratic * 0.2) * (dist * dist));    
     // spotlight intensity (soft edges)
     float theta = dot(lightDir, normalize(-light.direction)); 
     float epsilon = light.cutOff - light.outerCutOff;
     float intensity = smoothstep(0.0, 1.0, (theta - light.outerCutOff) / epsilon);
     
     vec3 diffuse = light.diffuse * diff * aColor * diffuseStrength * attenuation * intensity;
     // Disable specular for spotlight to avoid plastic-looking flat walls
     return diffuse;
 }
 
 void main()
 {
     // Compute world coordinates and normals
     vec3 FragPos = vec3(model * vec4(aPos, 1.0));
     vec3 Normal = mat3(transpose(inverse(model))) * aNormal;
     Normal = normalize(Normal);
     vec3 viewDir = normalize(viewPos - FragPos);
     
     // Stronger global ambient ensures shadows aren't pitch black and colors remain visible
     vec3 result = aColor * vec3(0.40); 
     
     if (useDirLight) {
         result += CalcDirLight(dirLight, Normal, viewDir);
     }
    
    for(int i = 0; i < numPointLights; i++) {
        result += CalcPointLight(pointLights[i], Normal, FragPos, viewDir);
    }
    
    if (useSpotLight) {
        result += CalcSpotLight(spotLight, Normal, FragPos, viewDir);
    }
    
    // Simple Exposure Tone Mapping to smoothly roll off bright spots and avoid neon/blown-out colors
    result = vec3(1.0) - exp(-result * 1.5);
    
    Color = result;
    TexCoords = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
