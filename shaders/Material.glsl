#ifndef _MATERIAL_GLSL_
#define _MATERIAL_GLSL_

#include "MaterialDefines.glsl"

struct Material {
  vec3 baseColor;
  vec3 normal;
  float metallic;  // [0..1]
  float roughness; // Perceptual [0..1]
  float specular;  // [0..1]
  vec3 emissiveColor;
#if BLEND_MODE == BLEND_MODE_TRANSPARENT
  float opacity; // [0..1]
#else            // OPAQUE/MASKED
  float ambientOcclusion; // [0..1]
#  if BLEND_MODE == BLEND_MODE_MASKED
  bool visible;           // Used to discard pixels
#  endif
#endif
};

Material _initMaterial() {
  Material material;
  material.baseColor = getVertexColor();
  material.normal = getVertexNormal();
  material.metallic = 0.5;
  material.roughness = 0.5;
  material.specular = 1.0;
  material.emissiveColor = vec3(0.0);
#if BLEND_MODE == BLEND_MODE_TRANSPARENT
  material.opacity = 1.0;
#else // OPAQUE or MASKED
  material.ambientOcclusion = 1.0;
#  if BLEND_MODE == BLEND_MODE_MASKED
  material.visible = true;
#  endif
#endif
  return material;
}

#pragma USER_SAMPLERS

void _executeUserCode(inout Material material) {
#pragma USER_CODE
}

#endif
