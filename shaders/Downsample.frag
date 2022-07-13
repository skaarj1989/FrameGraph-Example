#version 460 core

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_0;

layout(location = 0) uniform int u_MipLevel;

#include "Lib/Math.glsl"
#include "Lib/Color.glsl"
#include "Lib/Texture.glsl"

float karisAverage(vec3 color) {
  const float luma = getLuminance(linearTosRGB(color)) * 0.25;
  return 1.0 / (1.0 + luma);
}

layout(location = 0) out vec3 FragColor;
void main() {
  vec2 srcTexelSize = getTexelSize(t_0);
  float x = srcTexelSize.x;
  float y = srcTexelSize.y;

  // Take 13 samples around current (e) texel:
  // a - b - c
  // - j - k -
  // d - e - f
  // - l - m -
  // g - h - i
  vec3 a =
    texture(t_0, vec2(v_TexCoord.x - 2.0 * x, v_TexCoord.y + 2.0 * y)).rgb;
  vec3 b = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y + 2.0 * y)).rgb;
  vec3 c =
    texture(t_0, vec2(v_TexCoord.x + 2.0 * x, v_TexCoord.y + 2.0 * y)).rgb;

  vec3 d = texture(t_0, vec2(v_TexCoord.x - 2.0 * x, v_TexCoord.y)).rgb;
  vec3 e = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y)).rgb;
  vec3 f = texture(t_0, vec2(v_TexCoord.x + 2.0 * x, v_TexCoord.y)).rgb;

  vec3 g =
    texture(t_0, vec2(v_TexCoord.x - 2.0 * x, v_TexCoord.y - 2.0 * y)).rgb;
  vec3 h = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y - 2.0 * y)).rgb;
  vec3 i =
    texture(t_0, vec2(v_TexCoord.x + 2.0 * x, v_TexCoord.y - 2.0 * y)).rgb;

  vec3 j = texture(t_0, vec2(v_TexCoord.x - x, v_TexCoord.y + y)).rgb;
  vec3 k = texture(t_0, vec2(v_TexCoord.x + x, v_TexCoord.y + y)).rgb;
  vec3 l = texture(t_0, vec2(v_TexCoord.x - x, v_TexCoord.y - y)).rgb;
  vec3 m = texture(t_0, vec2(v_TexCoord.x + x, v_TexCoord.y - y)).rgb;

  vec3 groups[5];
  if (u_MipLevel == 0) {
    groups[0] = (a + b + d + e) * (0.125 / 4.0);
    groups[1] = (b + c + e + f) * (0.125 / 4.0);
    groups[2] = (d + e + g + h) * (0.125 / 4.0);
    groups[3] = (e + f + h + i) * (0.125 / 4.0);
    groups[4] = (j + k + l + m) * (0.5 / 4.0);
    groups[0] *= karisAverage(groups[0]);
    groups[1] *= karisAverage(groups[1]);
    groups[2] *= karisAverage(groups[2]);
    groups[3] *= karisAverage(groups[3]);
    groups[4] *= karisAverage(groups[4]);
    FragColor = groups[0] + groups[1] + groups[2] + groups[3] + groups[4];
    FragColor = max(FragColor, EPSILON);
  } else {
    FragColor = e * 0.125;
    FragColor += (a + c + g + i) * 0.03125;
    FragColor += (b + d + f + h) * 0.0625;
    FragColor += (j + k + l + m) * 0.125;
  }
}
