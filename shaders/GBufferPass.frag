#version 460 core

// Use with Geometry.vert

#include "BasePassAttributes.glsl"

#include <Lib/Math.glsl>
#include <Lib/Color.glsl>
#include <Lib/NormalMapping.glsl>

#include <Material.glsl>

#if BLEND_MODE == BLEND_MODE_OPAQUE
// https://www.khronos.org/opengl/wiki/Early_Fragment_Test
layout(early_fragment_tests) in;
#endif

layout(location = 3) uniform int u_MaterialFlags = 0;

layout(location = 0) out vec3 GBuffer0; // .rgb = Normal
layout(location = 1) out vec4 GBuffer1; // .rgb = Albedo, .a = SpecularWeight
layout(location = 2) out vec3 GBuffer2; // .rgb = Emissive
// .r = Metallic, .g = Roughness, .b = AO, .a = ShadingModel/MaterialFlags
layout(location = 3) out vec4 GBuffer3;

void main() {
  Material material = _initMaterial();
  _executeUserCode(material);

#if BLEND_MODE == BLEND_MODE_MASKED
  if (!material.visible) {
    discard;
    return;
  }
#endif

  GBuffer0.rgb = normalize(material.normal);
  GBuffer1 = vec4(material.baseColor, material.specular);
  GBuffer2.rgb = material.emissiveColor;

  int encoded = bitfieldInsert(0, SHADING_MODEL, 0, 2);
  encoded = bitfieldInsert(encoded, u_MaterialFlags, 2, 6);

  GBuffer3 = vec4(clamp01(material.metallic), clamp01(material.roughness),
                  clamp01(material.ambientOcclusion), float(encoded) / 255.0);
}
