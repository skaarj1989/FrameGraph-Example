#version 460 core

layout(location = 0) in VertexData {
  vec2 texCoord;
  vec4 color;
}
fs_in;

layout(binding = 0) uniform sampler2D t_0;

layout(location = 0) out vec4 FragColor;
void main() { FragColor = fs_in.color * texture(t_0, fs_in.texCoord); }
