#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool isEmissive;    // True for flames/glows — bypasses lighting
uniform float alpha;        // Alpha for transparency (default 1.0)

void main() {
    if (isEmissive) {
        // Emissive: full brightness, no lighting, use alpha for transparency
        FragColor = vec4(objectColor, alpha);
    } else {
        vec3 lightColor = vec3(1.0);
        float ambientStrength = 0.4;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }
}
