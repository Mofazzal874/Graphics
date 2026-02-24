#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexLightColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Texture mode: 0=none, 1=pure texture, 2=vertex-blended, 3=fragment-blended
uniform int textureMode;

// --- Light uniforms duplicated for vertex (Gouraud) lighting ---
uniform vec3 objectColor;
uniform vec3 viewPos;
uniform float shininess;

// Directional light
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;
uniform bool dirLightOn;

// Point lights
struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform bool pointLightsOn;

// Spot light
struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutOff;
    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;
uniform bool spotLightOn;

// Component toggles
uniform bool ambientOn;
uniform bool diffuseOn;
uniform bool specularOn;

// === Vertex-shader Phong (Gouraud) functions ===
vec3 CalcDirLightV(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    vec3 ambient = ambientOn ? light.ambient * objectColor : vec3(0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseOn ? light.diffuse * diff * objectColor : vec3(0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
    return ambient + diffuse + specular;
}

vec3 CalcPointLightV(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientOn ? light.ambient * objectColor : vec3(0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseOn ? light.diffuse * diff * objectColor : vec3(0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLightV(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientOn ? light.ambient * objectColor * attenuation : vec3(0.0);
    if (theta > light.cutOff) {
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diffuseOn ? light.diffuse * diff * objectColor : vec3(0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = specularOn ? light.specular * spec : vec3(0.0);
        return ambient + (diffuse + specular) * attenuation;
    }
    return ambient;
}

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);

    // Compute Gouraud lighting only when textureMode == 2
    VertexLightColor = vec3(1.0);
    if (textureMode == 2) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 result = vec3(0.0);
        if (dirLightOn)
            result += CalcDirLightV(dirLight, norm, viewDir);
        if (pointLightsOn) {
            for (int i = 0; i < NR_POINT_LIGHTS; i++)
                result += CalcPointLightV(pointLights[i], norm, FragPos, viewDir);
        }
        if (spotLightOn)
            result += CalcSpotLightV(spotLight, norm, FragPos, viewDir);
        VertexLightColor = clamp(result, 0.0, 1.0);
    }
}
