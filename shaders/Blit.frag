#version 460 core

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_0;

layout(location = 0) out vec3 FragColor;
void main() { FragColor = texture(t_0, v_TexCoord).rgb; }
