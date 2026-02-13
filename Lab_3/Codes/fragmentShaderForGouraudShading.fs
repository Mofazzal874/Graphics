#version 330 core

in vec4 LightingColor;
out vec4 FragColor;

void main()
{
    FragColor = LightingColor;
}
