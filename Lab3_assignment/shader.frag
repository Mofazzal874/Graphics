#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexLightColor;
in vec3 WorldPos;

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

// ==================== TEXTURE UNIFORMS ====================
uniform sampler2D textureSampler;
uniform sampler2D textureSampler2;  // Second texture for blending
// textureMode: 0=none, 1=pure texture, 2=vertex-blended (Gouraud), 3=fragment-blended (Phong), 4=multi-texture blend
uniform int textureMode;

// Blending parameters for textureMode 4
uniform float blendWidth;   // Width of transition zone
uniform float blendEdge;    // World-space coordinate of blend center
uniform int blendAxis;      // 0=X, 1=Y, 2=Z

// ==================== LIGHT CALCULATION FUNCTIONS ====================

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 matColor) {
    vec3 lightDir = normalize(-light.direction);
    vec3 ambient = ambientOn ? light.ambient * matColor : vec3(0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseOn ? light.diffuse * diff * matColor : vec3(0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientOn ? light.ambient * matColor : vec3(0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseOn ? light.diffuse * diff * matColor : vec3(0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientOn ? light.ambient * matColor * attenuation : vec3(0.0);
    if (theta > light.cutOff) {
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diffuseOn ? light.diffuse * diff * matColor : vec3(0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
        diffuse  *= attenuation;
        specular *= attenuation;
        return ambient + diffuse + specular;
    } else {
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
    
    // Determine material color based on texture mode
    vec3 matColor = objectColor;
    vec3 texColor = vec3(1.0);
    
    if (textureMode == 1) {
        // Mode 1: Pure texture — texture replaces object color entirely
        texColor = texture(textureSampler, TexCoord).rgb;
        // Compute lighting with texture as material
        vec3 result = vec3(0.0);
        if (dirLightOn)
            result += CalcDirLight(dirLight, norm, viewDir, texColor);
        if (pointLightsOn) {
            for (int i = 0; i < NR_POINT_LIGHTS; i++)
                result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, texColor);
        }
        if (spotLightOn)
            result += CalcSpotLight(spotLight, norm, FragPos, viewDir, texColor);
        result = clamp(result, 0.0, 1.0);
        FragColor = vec4(result, alpha);
        return;
    }
    
    if (textureMode == 2) {
        // Mode 2: Texture × vertex-computed (Gouraud) lighting
        texColor = texture(textureSampler, TexCoord).rgb;
        vec3 result = texColor * VertexLightColor;
        result = clamp(result, 0.0, 1.0);
        FragColor = vec4(result, alpha);
        return;
    }
    
    if (textureMode == 3) {
        // Mode 3: Blended texture × fragment-computed (Phong) lighting
        texColor = texture(textureSampler, TexCoord).rgb;
        // Blend texture color with object color (70% texture, 30% object color)
        vec3 blendedMat = mix(objectColor, texColor, 0.7);
        // Compute per-fragment Phong with blended material
        vec3 phongResult = vec3(0.0);
        if (dirLightOn)
            phongResult += CalcDirLight(dirLight, norm, viewDir, blendedMat);
        if (pointLightsOn) {
            for (int i = 0; i < NR_POINT_LIGHTS; i++)
                phongResult += CalcPointLight(pointLights[i], norm, FragPos, viewDir, blendedMat);
        }
        if (spotLightOn)
            phongResult += CalcSpotLight(spotLight, norm, FragPos, viewDir, blendedMat);
        vec3 result = clamp(phongResult, 0.0, 1.0);
        FragColor = vec4(result, alpha);
        return;
    }
    
    if (textureMode == 4) {
        // Mode 4: Multi-texture blend with smooth transition
        vec3 tex1Color = texture(textureSampler, TexCoord).rgb;
        vec3 tex2Color = texture(textureSampler2, TexCoord).rgb;
        
        // Get world-space coordinate along the blend axis
        float coord = WorldPos.x;
        if (blendAxis == 1) coord = WorldPos.y;
        if (blendAxis == 2) coord = WorldPos.z;
        
        // Smooth transition using smoothstep
        float absCoord = abs(coord);
        float blendFactor = smoothstep(blendEdge - blendWidth, blendEdge + blendWidth, absCoord);
        
        // Blend from tex1 (road, near center) to tex2 (grass, far from center)
        vec3 blendedTex = mix(tex1Color, tex2Color, blendFactor);
        // Also blend with object color for realism
        vec3 blendedMat = mix(objectColor, blendedTex, 0.75);
        
        // Compute per-fragment Phong with blended material
        vec3 phongResult = vec3(0.0);
        if (dirLightOn)
            phongResult += CalcDirLight(dirLight, norm, viewDir, blendedMat);
        if (pointLightsOn) {
            for (int i = 0; i < NR_POINT_LIGHTS; i++)
                phongResult += CalcPointLight(pointLights[i], norm, FragPos, viewDir, blendedMat);
        }
        if (spotLightOn)
            phongResult += CalcSpotLight(spotLight, norm, FragPos, viewDir, blendedMat);
        vec3 result = clamp(phongResult, 0.0, 1.0);
        FragColor = vec4(result, alpha);
        return;
    }
    
    // Mode 0: No texture — original Phong lighting
    vec3 result = vec3(0.0);
    if (dirLightOn)
        result += CalcDirLight(dirLight, norm, viewDir, objectColor);
    if (pointLightsOn) {
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, objectColor);
    }
    if (spotLightOn)
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, objectColor);
    result = clamp(result, 0.0, 1.0);
    FragColor = vec4(result, alpha);
}
