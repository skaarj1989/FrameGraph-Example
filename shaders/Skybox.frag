#version 460 core

layout(location = 0) in Interface {
  vec3 eyeDirection;
  vec2 texCoord;
}
fs_in;

layout(binding = 0) uniform samplerCube t_Skybox;

layout(location = 0) out vec3 FragColor;
void main() { FragColor = texture(t_Skybox, fs_in.eyeDirection).rgb; }
