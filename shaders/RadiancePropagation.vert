#version 460 core

#include <Lib/Math.glsl>

layout(location = 0) uniform vec3 u_GridSize;

layout(location = 0) out VertexData { flat ivec3 cellIndex; }
vs_out;

void main() {
  const vec3 position = vec3(unflatten3D(gl_VertexID, uvec2(u_GridSize.xy)));

  vs_out.cellIndex = ivec3(position);

  const vec2 ndc = (position.xy + 0.5) / u_GridSize.xy * 2.0 - 1.0;
  gl_Position = vec4(ndc, 0.0, 1.0);

  gl_PointSize = 1.0;
}
