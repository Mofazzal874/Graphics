#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform float rotation;    // Rotation angle in radians
uniform float scale;       // Scale factor
uniform vec2 translation;  // Translation offset

void main()
{
    // Apply scale first
    float scaledX = aPos.x * scale;
    float scaledY = aPos.y * scale;
    
    // Then apply rotation
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float rotatedX = scaledX * cosR - scaledY * sinR;
    float rotatedY = scaledX * sinR + scaledY * cosR;
    
    // Finally apply translation
    float finalX = rotatedX + translation.x;
    float finalY = rotatedY + translation.y;
    
    gl_Position = vec4(finalX, finalY, aPos.z, 1.0);
    ourColor = aColor;
}
