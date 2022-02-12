#version 460 core

// https://www.shadertoy.com/view/lsKSWR

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D t_SceneColor;

layout(location = 0) out vec3 FragColor;
void main() {
  vec2 uv = v_TexCoord;
  uv *= 1.0 - uv.yx;
  float vig = uv.x * uv.y * 15.0;
  vig = pow(vig, 0.1);

  FragColor = texture(t_SceneColor, v_TexCoord).rgb * vig;
}
