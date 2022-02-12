#version 460 core

// https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_0;

#include <Lib/Texture.glsl>

layout(location = 0) uniform bool u_Horizontal;
layout(location = 1) uniform float u_Scale = 1.0;

const float kOffsets[3] = {0.0, 1.3846153846, 3.2307692308};
const float kWeights[3] = {0.2270270270, 0.3162162162, 0.0702702703};

vec3 horizontalBlur() {
  const float texOffset = getTexelSize(t_0).x * u_Scale;
  vec3 result = texture(t_0, v_TexCoord).rgb * kWeights[0];
  for (uint i = 1; i < 3; ++i) {
    result +=
      texture(t_0, v_TexCoord + vec2(texOffset * i, 0.0)).rgb * kWeights[i];
    result +=
      texture(t_0, v_TexCoord - vec2(texOffset * i, 0.0)).rgb * kWeights[i];
  }
  return result;
}
vec3 verticalBlur() {
  const float texOffset = getTexelSize(t_0).y * u_Scale;
  vec3 result = texture(t_0, v_TexCoord).rgb * kWeights[0];
  for (uint i = 1; i < 3; ++i) {
    result +=
      texture(t_0, v_TexCoord + vec2(0.0, texOffset * i)).rgb * kWeights[i];
    result +=
      texture(t_0, v_TexCoord - vec2(0.0, texOffset * i)).rgb * kWeights[i];
  }
  return result;
}

layout(location = 0) out vec3 FragColor;
void main() { FragColor = u_Horizontal ? horizontalBlur() : verticalBlur(); }
