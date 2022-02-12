#version 460 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;

layout(location = 0) uniform mat4 u_ProjectionMatrix = mat4(1.0);

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out VertexData {
  vec2 texCoord;
  vec4 color;
}
vs_out;

void main() {
  vs_out.texCoord = a_TexCoord;
  vs_out.color = a_Color;
  gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
}
