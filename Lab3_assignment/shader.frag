#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// ==================== LIGHT STRUCTS ====================
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutOff;       // cos(cutoff angle)
    float constant;
    float linear;
    float quadratic;
};

// ==================== UNIFORMS ====================
#define NR_POINT_LIGHTS 4

uniform vec3 objectColor;
uniform vec3 viewPos;

// Material
uniform float shininess;     // Specular exponent (e.g. 32.0)

// Lights
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

// Toggle switches — light types
uniform bool dirLightOn;
uniform bool pointLightsOn;
uniform bool spotLightOn;

// Toggle switches — light components
uniform bool ambientOn;
uniform bool diffuseOn;
uniform bool specularOn;

// Emissive mode (for flames, glows)
uniform bool isEmissive;
uniform float alpha;

// ==================== LIGHT CALCULATION FUNCTIONS ====================

// Phong model: per-fragment lighting
// Each function computes ambient + diffuse + specular for its light type

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    
    // Ambient
    vec3 ambient = vec3(0.0);
    if (ambientOn)
        ambient = light.ambient * objectColor;
    
    // Diffuse (Lambert's cosine law)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vec3(0.0);
    if (diffuseOn)
        diffuse = light.diffuse * diff * objectColor;
    
    // Specular (Phong reflection model)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = vec3(0.0);
    if (specularOn)
        specular = light.specular * spec;
    
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Attenuation (inverse-square falloff)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Ambient
    vec3 ambient = vec3(0.0);
    if (ambientOn)
        ambient = light.ambient * objectColor;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vec3(0.0);
    if (diffuseOn)
        diffuse = light.diffuse * diff * objectColor;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = vec3(0.0);
    if (specularOn)
        specular = light.specular * spec;
    
    // Apply attenuation
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Spot light cone — single cutoff angle
    float theta = dot(lightDir, normalize(-light.direction));
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Ambient (always present inside or outside cone)
    vec3 ambient = vec3(0.0);
    if (ambientOn)
        ambient = light.ambient * objectColor * attenuation;
    
    if (theta > light.cutOff) {
        // Inside the spotlight cone
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = vec3(0.0);
        if (diffuseOn)
            diffuse = light.diffuse * diff * objectColor;
        
        // Specular
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = vec3(0.0);
        if (specularOn)
            specular = light.specular * spec;
        
        diffuse  *= attenuation;
        specular *= attenuation;
        
        return ambient + diffuse + specular;
    } else {
        // Outside cone — only ambient
        return ambient;
    }
}

// ==================== MAIN ====================
void main() {
    if (isEmissive) {
        // Emissive objects bypass all lighting (flames, glows)
        FragColor = vec4(objectColor, alpha);
        return;
    }
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    // 1. Directional light
    if (dirLightOn)
        result += CalcDirLight(dirLight, norm, viewDir);
    
    // 2. Point lights
    if (pointLightsOn) {
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }
    
    // 3. Spot light
    if (spotLightOn)
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    // Clamp to avoid oversaturation
    result = clamp(result, 0.0, 1.0);
    
    FragColor = vec4(result, alpha);
}
