#version 460 core

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_SceneColor;

layout(location = 0) uniform float u_Threshold = 1.0;

#include <Lib/Color.glsl>
#include <Lib/Texture.glsl>

// https://catlikecoding.com/unity/tutorials/custom-srp/hdr/

vec3 prefilterFireflies() {
  const vec2 kOffsets[] = {
    vec2(0.0, 0.0),  vec2(-1.0, -1.0), vec2(-1.0, 1.0),
    vec2(1.0, -1.0), vec2(1.0, 1.0),
  };
  const vec2 texelSize = getTexelSize(t_SceneColor);

  vec3 color = vec3(0.0);
  float weightSum = 0;
  for (uint i = 0; i < 5; ++i) {
    const vec3 c =
      texture(t_SceneColor, v_TexCoord + (texelSize * kOffsets[i] * 2.0)).rgb;
    const float w = 1.0 / (getLuminance(c) + 1.0);
    color += c * w;
    weightSum += w;
  }
  return color / weightSum;
}

layout(location = 0) out vec3 BrightColor;
void main() {
  const vec3 color = texture(t_SceneColor, v_TexCoord).rgb;
  if (getLuminance(color) > u_Threshold)
    BrightColor = prefilterFireflies();
  else
    discard;
}
