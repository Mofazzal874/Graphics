#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec4 LightingColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    //float k_c;  // attenuation factors
    //float k_l;  // attenuation factors
    //float k_q;  // attenuation factors  
};


#define NR_POINT_LIGHTS 1

uniform vec3 viewPos;
uniform PointLight pointLight;
uniform Material material;

// function prototypes
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 Pos, vec3 V);

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    vec3 Pos = vec3(model * vec4(aPos, 1.0));
    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // properties
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - Pos);

    vec3 result = CalcPointLight(material, pointLight, N, Pos, V);
   
    LightingColor = vec4(result, 1.0);
    
}



// calculates the color when using a point light.
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 Pos, vec3 V)
{
    vec3 L = normalize(light.position - Pos);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    
    // attenuation
    float d = length(light.position - Pos);
    //float attenuation = ;
    
    vec3 ambient = K_A * light.ambient;
    vec3 diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    vec3 specular = K_S * pow(max(dot(V, R), 0.0), material.shininess) * light.specular;
    
    //ambient *= attenuation;
    //diffuse *= attenuation;
    //specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

