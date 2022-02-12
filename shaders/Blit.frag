#version 460 core

layout(location = 0) in vec2 v_TexCoord;

#include <Resources/FrameBlock.glsl>

layout(binding = 0) uniform sampler2D t_0;

const uint Mode_Default = 0;
const uint Mode_LinearDepth = 1;
const uint Mode_RedChannel = 2;
const uint Mode_GreenChannel = 3;
const uint Mode_BlueChannel = 4;
const uint Mode_AlphaChannel = 5;
const uint Mode_WorldSpaceNormals = 6;

layout(location = 0) uniform uint u_Mode = Mode_Default;

#include <Lib/Depth.glsl>

layout(location = 0) out vec3 FragColor;
void main() {
  const vec4 source = texture(t_0, v_TexCoord);

  switch (u_Mode) {
  case Mode_LinearDepth:
    FragColor = vec3(linearizeDepth(source.r) / u_Frame.camera.far);
    break;
  case Mode_RedChannel:
    FragColor = source.rrr;
    break;
  case Mode_GreenChannel:
    FragColor = source.ggg;
    break;
  case Mode_BlueChannel:
    FragColor = source.bbb;
    break;
  case Mode_AlphaChannel:
    FragColor = source.aaa;
    break;
  case Mode_WorldSpaceNormals:
    FragColor = vec3(viewToWorld(vec4(source.rgb, 0.0)));
    break;

  case Mode_Default:
  default:
    FragColor = source.rgb;
    break;
  }
}
