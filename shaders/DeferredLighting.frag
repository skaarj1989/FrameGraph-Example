#version 460 core

layout(location = 0) in vec2 v_TexCoord;

#include <Resources/FrameBlock.glsl>

layout(binding = 0) uniform sampler2D t_SceneDepth;

layout(binding = 1) uniform sampler2D t_GBuffer0; // .rgb = Normal
layout(binding = 2) uniform sampler2D
  t_GBuffer1; // .rgb = Albedo, .a = SpecularWeight
layout(binding = 3) uniform sampler2D t_GBuffer2; // .rgb = EmissiveColor
// .r = Metallic, .g = Roughness, .b = AO, .a = ShadingModel/MaterialFlags
layout(binding = 4) uniform sampler2D t_GBuffer3;

layout(binding = 5) uniform sampler2D t_SSAO;

layout(binding = 6) uniform sampler2D t_BRDF;
layout(binding = 7) uniform samplerCube t_IrradianceMap;
layout(binding = 8) uniform samplerCube t_PrefilteredEnvMap;

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

layout(binding = 9) uniform sampler2DArrayShadow t_CascadedShadowMaps;

layout(binding = 10) uniform sampler3D t_AccumulatedSH_R;
layout(binding = 11) uniform sampler3D t_AccumulatedSH_G;
layout(binding = 12) uniform sampler3D t_AccumulatedSH_B;

#include <Lib/Depth.glsl>

#include <Lib/Light.glsl>
_DECLARE_LIGHT_BUFFER(0, g_LightBuffer);

#include <Lib/IBL_AmbientLighting.glsl>
#include <Lib/PBR_DirectLighting.glsl>

#define SOFT_SHADOWS 1
#include <Lib/CSM.glsl>
#include "LPV.glsl"

layout(location = 12) uniform vec3 u_MinCorner;
layout(location = 13) uniform vec3 u_GridSize;
layout(location = 14) uniform float u_CellSize;

#include <MaterialDefines.glsl>

layout(location = 0) out vec3 FragColor;
void main() {
  const float depth = getDepth(t_SceneDepth, v_TexCoord);
  if (depth >= 1.0) discard;

  const vec3 emissiveColor = texture(t_GBuffer2, v_TexCoord).rgb;

  vec4 temp = texture(t_GBuffer3, v_TexCoord);

  const int encoded = int(temp.a * 255.0);
  const int shadingModel = bitfieldExtract(encoded, 0, 2);
  if (shadingModel == SHADING_MODEL_UNLIT) {
    FragColor = emissiveColor;
    return;
  }
  const int materialFlags = bitfieldExtract(encoded, 2, 6);
  const bool receiveShadow =
    (materialFlags & MaterialFlag_ReceiveShadow) == MaterialFlag_ReceiveShadow;

  const float metallic = temp.r;
  const float roughness = temp.g;
  float ao = temp.b;
  if (hasRenderFeatures(RenderFeature_SSAO)) {
    // ao = min(ao, texture(t_SSAO, v_TexCoord).r);
    ao = texture(t_SSAO, v_TexCoord).r;
  }

  temp = texture(t_GBuffer1, v_TexCoord);
  const vec3 albedo = temp.rgb;
  const float specularWeight = temp.a;

  //
  // Lighting calculation:
  //

  // NOTE: values in world-space
  // N = normal (from surface)
  // L = fragment to light direction
  // V = fragment to eye direction
  // H = halfway vector (between V and L)

  const vec3 fragPosViewSpace = viewPositionFromDepth(depth, v_TexCoord);
  const vec3 fragPosWorldSpace =
    (u_Frame.camera.inversedView * vec4(fragPosViewSpace, 1.0)).xyz;

  const vec3 N = normalize(texture(t_GBuffer0, v_TexCoord).rgb);
  const vec3 V = normalize(getCameraPosition() - fragPosWorldSpace);
  const float NdotV = clamp01(dot(N, V));

  // Dielectrics: [0.02..0.05], Metals: [0.5..1.0]
  const float kMinRoughness = 0.04;
  vec3 F0 = vec3(kMinRoughness);
  const vec3 diffuseColor = mix(albedo * (1.0 - F0), vec3(0.0), metallic);
  F0 = mix(F0, albedo, metallic);

  const float alphaRoughness = roughness * roughness;

  //
  // Ambient lighting:
  //

  vec3 Lo_diffuse = vec3(0.0);
  vec3 Lo_specular = vec3(0.0);

#if 0
  // clang-format off
  const LightContribution ambientLighting = IBL_AmbientLighting(
    diffuseColor, F0, specularWeight, roughness, N, V, NdotV);
  // clang-format on
   Lo_diffuse = ambientLighting.diffuse * ao;
   Lo_specular = ambientLighting.specular * ao;
#endif

  if (hasRenderFeatures(RenderFeature_GI)) {
    const vec3 cellCoords =
      (fragPosWorldSpace - u_MinCorner) / u_CellSize / u_GridSize;

    // clang-format off
    const SHcoeffs coeffs = {
      texture(t_AccumulatedSH_R, cellCoords, 0),
      texture(t_AccumulatedSH_G, cellCoords, 0),
      texture(t_AccumulatedSH_B, cellCoords, 0)
    };
    // clang-format on

    const vec4 SH_intensity = SH_evaluate(-N);
    const vec3 LPV_intensity =
      vec3(dot(SH_intensity, coeffs.red), dot(SH_intensity, coeffs.green),
           dot(SH_intensity, coeffs.blue));

    const vec3 LPV_radiance =
      max(LPV_intensity * 4 / u_CellSize / u_CellSize, 0.0);

    Lo_diffuse += albedo * LPV_radiance * ao;

    if (hasDebugFlags(DebugFlag_RadianceOnly)) {
      FragColor = LPV_radiance;
      return;
    }
  }

  //
  // Direct lighting:
  //

#if TILED_LIGHTING
  const ivec2 tileId = ivec2(floor(gl_FragCoord.xy / TILE_SIZE));
  const uvec2 tileInfo = imageLoad(i_LightGrid, tileId).rg;
  const uint startOffset = tileInfo.x;
  const uint lightCount = tileInfo.y;

  for (uint i = 0; i < lightCount; ++i) {
    const uint lightIndex = g_LightIndexList[startOffset + i].x;
#else
  for (uint i = 0; i < g_LightBuffer.numLights; ++i) {
    const uint lightIndex = i;
#endif
    const Light light = g_LightBuffer.data[lightIndex];

    const vec3 fragToLight = light.type != LightType_Directional
                               ? light.position.xyz - fragPosWorldSpace
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
            _getDirLightVisibility(cascadeIndex, fragPosWorldSpace, NdotL);
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
        specularWeight,
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

  FragColor = Lo_diffuse + Lo_specular + emissiveColor;
}
