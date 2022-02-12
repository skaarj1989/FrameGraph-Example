#version 460 core

layout(location = 0) in vec2 v_TexCoord;

#include <Resources/FrameBlock.glsl>
#include <Resources/Cascades.glsl>

layout(binding = 0) uniform sampler2D t_SceneDepth;

#include <Lib/Depth.glsl>

layout(location = 0) out vec4 FragColor;
void main() {
  if (!hasRenderFeatures(RenderFeature_Shadows)) {
    discard;
    return;
  }
  const float depth = getDepth(t_SceneDepth, v_TexCoord);
  if (depth >= 1.0) {
    // Don't touch the far plane
    discard;
    return;
  }

  const vec3 fragPosViewSpace = viewPositionFromDepth(depth, v_TexCoord);
  switch (_selectCascadeIndex(fragPosViewSpace)) {
  case 0:
    FragColor.rgb = vec3(1.00, 0.25, 0.25);
    break;
  case 1:
    FragColor.rgb = vec3(0.25, 1.00, 0.25);
    break;
  case 2:
    FragColor.rgb = vec3(0.25, 0.25, 1.00);
    break;
  case 3:
    FragColor.rgb = vec3(1.00, 1.00, 0.25);
    break;
  }
  FragColor.a = 0.35; // Intensity
}
