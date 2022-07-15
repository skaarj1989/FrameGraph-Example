#version 460 core

layout(binding = 0) uniform sampler2D t_GBuffer0; // .rgb = world position
layout(binding = 1) uniform sampler2D t_GBuffer1; // .rgb = world normal

#include "LPV.glsl"

layout(location = 0) uniform int u_RSMResolution;
layout(location = 1) uniform vec3 u_MinCorner;
layout(location = 2) uniform vec3 u_GridSize;
layout(location = 3) uniform float u_CellSize;

layout(location = 4) uniform mat4 u_VP;

layout(location = 0) out VertexData {
  vec3 cellCoord;
  vec3 N;
}
vs_out;

void main() {
  const ivec3 id =
    ivec3(unflatten3D(gl_VertexID, uvec2(u_GridSize.xy))); // [0..GridSize]

  const ivec2 coord = ivec2(unflatten2D(gl_VertexID, u_RSMResolution));
  const vec3 position = texelFetch(t_GBuffer0, coord.xy, 0).rgb;
  const vec3 N = texelFetch(t_GBuffer1, coord.xy, 0).rgb;

  vs_out.cellCoord = (position - u_MinCorner) / u_CellSize / u_GridSize;
  vs_out.N = N;

  gl_Position = u_VP * vec4(position + 0.5 * N, 1.0);

  gl_PointSize = 10.0;
}
