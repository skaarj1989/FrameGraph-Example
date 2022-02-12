#version 460 core

#include <Resources/FrameBlock.glsl>
#include <Lib/SpaceTraversal.glsl>

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out Interface {
  vec3 eyeDirection;
  vec2 texCoord;
}
vs_out;

void main() {
  vs_out.texCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  gl_Position = vec4(vs_out.texCoord * 2.0 - 1.0, 1.0, 1.0);
  vs_out.eyeDirection = vec3(viewToWorld(vec4(clipToView(gl_Position), 0.0)));
  gl_Position.z = gl_Position.w * 0.999999;
}
