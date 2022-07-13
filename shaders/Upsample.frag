#version 460 core

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_0;

layout(location = 0) uniform float u_Radius;

layout(location = 0) out vec3 FragColor;
void main() {
  float x = u_Radius;
  float y = u_Radius;

  // Take 9 samples around current (e) texel:
  // a - b - c
  // d - e - f
  // g - h - i
  vec3 a = texture(t_0, vec2(v_TexCoord.x - x, v_TexCoord.y + y)).rgb;
  vec3 b = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y + y)).rgb;
  vec3 c = texture(t_0, vec2(v_TexCoord.x + x, v_TexCoord.y + y)).rgb;

  vec3 d = texture(t_0, vec2(v_TexCoord.x - x, v_TexCoord.y)).rgb;
  vec3 e = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y)).rgb;
  vec3 f = texture(t_0, vec2(v_TexCoord.x + x, v_TexCoord.y)).rgb;

  vec3 g = texture(t_0, vec2(v_TexCoord.x - x, v_TexCoord.y - y)).rgb;
  vec3 h = texture(t_0, vec2(v_TexCoord.x, v_TexCoord.y - y)).rgb;
  vec3 i = texture(t_0, vec2(v_TexCoord.x + x, v_TexCoord.y - y)).rgb;

  // Apply weighted distribution, by using a 3x3 tent filter:
  //  1   | 1 2 1 |
  // -- * | 2 4 2 |
  // 16   | 1 2 1 |
  FragColor = e * 4.0;
  FragColor += (b + d + f + h) * 2.0;
  FragColor += (a + c + g + i);
  FragColor *= 1.0 / 16.0;
}
