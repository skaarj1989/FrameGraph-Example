#version 460 core

// Use with Geometry.vert

#include <Resources/FrameBlock.glsl>
#include "BasePassAttributes.glsl"

#include <Lib/Math.glsl>
#include <Lib/Color.glsl>
#include <Lib/NormalMapping.glsl>

#include <Material.glsl>

layout(location = 12) uniform int u_MaterialFlags = 0;
layout(location = 13) uniform vec3 u_LightIntensity;

#if BLEND_MODE == BLEND_MODE_OPAQUE
// https://www.khronos.org/opengl/wiki/Early_Fragment_Test
layout(early_fragment_tests) in;
#endif

layout(location = 0) out vec3 GBuffer0; // .rgb = world position
layout(location = 1) out vec3 GBuffer1; // .rgb = normal
layout(location = 2) out vec3 GBuffer2; // .rgb = flux

void main() {
  Material material = _initMaterial();
  _executeUserCode(material);

#if BLEND_MODE == BLEND_MODE_MASKED
  if (!material.visible) discard;
#endif

  GBuffer0.rgb = fs_in.fragPos.xyz;
  GBuffer1.rgb = normalize(material.normal);
  GBuffer2.rgb =
    (material.baseColor * u_LightIntensity) + material.emissiveColor;
}
