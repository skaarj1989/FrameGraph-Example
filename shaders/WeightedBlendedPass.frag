#version 460 core

// Use with Geometry.vert

#include <Resources/FrameBlock.glsl>

#include "BasePassAttributes.glsl"

layout(binding = 0) uniform sampler2D t_SceneDepth;

layout(binding = 1) uniform sampler2D t_BRDF;
layout(binding = 2) uniform samplerCube t_IrradianceMap;
layout(binding = 3) uniform samplerCube t_PrefilteredEnvMap;

#if TILED_LIGHTING
layout(binding = 1, std430) restrict readonly buffer LightIndexList {
  // .x = indices for opaque geometry
  // .y = indices for translucent geometry
  uvec2 g_LightIndexList[];
};

// [startOffset, lightCount]
// .rg = opaque geometry
// .ba = translucent geometry
layout(binding = 0, rgba32ui) restrict readonly uniform uimage2D i_LightGrid;
#endif

layout(binding = 4) uniform sampler2DArrayShadow t_CascadedShadowMaps;

#include <Lib/Depth.glsl>

#include <Lib/Light.glsl>
_DECLARE_LIGHT_BUFFER(0, g_LightBuffer);

#include <Lib/IBL_AmbientLighting.glsl>
#include <Lib/PBR_DirectLighting.glsl>

#define SOFT_SHADOWS 1
#include <Lib/CSM.glsl>

#include <Lib/Color.glsl>
#include <Lib/NormalMapping.glsl>

#include <Material.glsl>

layout(location = 12) uniform int u_MaterialFlags = 0;

layout(location = 0) out vec4 Accum;
layout(location = 1) out float Reveal;

void main() {
  Material material = _initMaterial();
  _executeUserCode(material);

#if SHADING_MODEL == SHADING_MODEL_UNLIT
  const vec3 color = material.emissiveColor;
#else
  const vec3 fragPosViewSpace =
    (u_Frame.camera.view * vec4(fs_in.fragPos.xyz, 1.0)).xyz;

  const vec3 N = normalize(material.normal);
  const vec3 V = normalize(getCameraPosition() - fs_in.fragPos.xyz);
  const float NdotV = clamp01(dot(N, V));

  const bool receiveShadow = (u_MaterialFlags & MaterialFlag_ReceiveShadow) ==
                             MaterialFlag_ReceiveShadow;

  const float kMinRoughness = 0.04;
  vec3 F0 = vec3(kMinRoughness);
  const vec3 diffuseColor =
    mix(material.baseColor * (1.0 - F0), vec3(0.0), material.metallic);
  F0 = mix(F0, material.baseColor, material.metallic);

  const float alphaRoughness = material.roughness * material.roughness;

  //
  // Ambient lighting:
  //

  vec3 Lo_diffuse = vec3(0.0);
  vec3 Lo_specular = vec3(0.0);

  if (hasRenderFeatures(RenderFeature_IBL)) {
    // clang-format off
    const LightContribution ambientLighting = IBL_AmbientLighting(
      diffuseColor,
      F0,
      material.specular,
      material.roughness,
      N,
      V,
      NdotV
    );
    // clang-format on
    Lo_diffuse = ambientLighting.diffuse;
    Lo_specular = ambientLighting.specular;
  }

  //
  // Direct lighting:
  //

#  if TILED_LIGHTING
  const ivec2 tileId = ivec2(floor(gl_FragCoord.xy / TILE_SIZE));
  const uvec2 tileInfo = imageLoad(i_LightGrid, tileId).ba;
  const uint startOffset = tileInfo.x;
  const uint lightCount = tileInfo.y;

  for (uint i = 0; i < lightCount; ++i) {
    const uint lightIndex = g_LightIndexList[startOffset + i].y;
#  else
  for (uint i = 0; i < g_LightBuffer.numLights; ++i) {
    const uint lightIndex = i;
#  endif
    const Light light = g_LightBuffer.data[lightIndex];

    const vec3 fragToLight = light.type != LightType_Directional
                               ? light.position.xyz - fs_in.fragPos.xyz
                               : -light.direction.xyz;

    const vec3 L = normalize(fragToLight);
    const vec3 H = normalize(V + L);

    const float NdotL = clamp01(dot(N, L));
    if (NdotL > 0.0 || NdotV > 0.0) {
      float visibility = 1.0;
      if (hasRenderFeatures(RenderFeature_Shadows) && receiveShadow) {
        if (light.type == LightType_Directional) {
          const uint cascadeIndex = _selectCascadeIndex(fragPosViewSpace);
          visibility =
            _getDirLightVisibility(cascadeIndex, fs_in.fragPos.xyz, NdotL);
        }

        if (visibility == 0.0) continue;
      }

      const vec3 radiance =
        _getLightIntensity(light, fragToLight) * NdotL * visibility;

      // clang-format off
      const LightContribution directLighting = PBR_DirectLighting(
        radiance,
        diffuseColor,
        F0,
        material.specular,
        alphaRoughness,
        NdotV,
        NdotL,
        clamp01(dot(N, H)),
        clamp01(dot(V, H))
      );
      // clang-format on

      Lo_diffuse += directLighting.diffuse;
      Lo_specular += directLighting.specular;
    }
  }

  const vec3 color = Lo_diffuse + Lo_specular + material.emissiveColor;
#endif

  const float a = clamp01(material.opacity);
  const float w = clamp(pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1e8 *
                          pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
                        1e-2, 3e3);

  Accum = vec4(color * a, a) * w;
  Reveal = a;
}
