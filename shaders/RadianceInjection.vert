#version 460 core

layout(binding = 0) uniform sampler2D t_GBuffer0; // .rgb = world position
layout(binding = 1) uniform sampler2D t_GBuffer1; // .rgb = world normal
layout(binding = 2) uniform sampler2D t_GBuffer2; // .rgb = flux

#include "LPV.glsl"

layout(location = 0) uniform int u_RSMResolution;
layout(location = 1) uniform vec3 u_MinCorner;
layout(location = 2) uniform vec3 u_GridSize;
layout(location = 3) uniform float u_CellSize;

layout(location = 0) out VertexData {
  vec3 N;
  vec4 flux;
  flat ivec3 cellIndex;
}
vs_out;

void main() {
  const ivec2 coord = ivec2(unflatten2D(gl_VertexID, u_RSMResolution));

  const vec3 position = texelFetch(t_GBuffer0, coord, 0).xyz;
  const vec3 N = texelFetch(t_GBuffer1, coord, 0).xyz;
  const vec4 flux = texelFetch(t_GBuffer2, coord, 0);

  vs_out.N = N;
  vs_out.flux = flux;

  const ivec3 gridCell = ivec3((position - u_MinCorner) / u_CellSize + 0.5 * N);
  vs_out.cellIndex = gridCell;

  const vec2 ndc = (vec2(gridCell.xy) + 0.5) / u_GridSize.xy * 2.0 - 1.0;
  gl_Position = vec4(ndc, 0.0, 1.0);

  gl_PointSize = 1.0;
}
