#ifndef _NORMAL_MAPPING_GLSL
#define _NORMAL_MAPPING_GLSL

#include "CotangentFrame.glsl"

vec3 sampleNormalMap(sampler2D normalMap, vec2 texCoord) {
  const vec3 N = texture(normalMap, texCoord).rgb * 2.0 - 1.0;
  return normalize(N);
}

// http://jbit.net/~sparky/sfgrad_bump/mm_sfgrad_bump.pdf

vec2 evaluateHeight(sampler2D bumpMap, vec2 texCoord, float scale) {
  const vec2 duv1 = dFdx(texCoord);
  const vec2 duv2 = dFdy(texCoord);
  const float Hll = texture(bumpMap, texCoord).r * scale;
  const float Hlr = texture(bumpMap, texCoord + duv1).r * scale;
  const float Hul = texture(bumpMap, texCoord + duv2).r * scale;
  return vec2(Hlr - Hll, Hul - Hll);
}

vec3 perturbNormal(vec3 N, vec2 dB) {
  const vec3 sigmaS = dFdx(fs_in.fragPos.xyz);
  const vec3 sigmaT = dFdy(fs_in.fragPos.xyz);
  const vec3 R1 = cross(sigmaT, N);
  const vec3 R2 = cross(N, sigmaS);
  const float det = dot(sigmaS, R1);
  const vec3 surfaceGradient = sign(det) * (dB.s * R1 + dB.t * R2);
  return normalize(abs(det) * N - surfaceGradient);
}

// Transform normal: tangent-space -> world-space
vec3 tangentToWorld(vec3 N, vec2 texCoord) {
#if !DEPTH_PASS
#  ifdef HAS_NORMAL
#    ifdef HAS_TANGENTS
  return normalize(fs_in.TBN * N);
#    else
  const mat3 TBN = cotangentFrame(getVertexNormal(), texCoord);
  return normalize(TBN * N);
#    endif
#  else
  return vec3(0.0);
#  endif
#else
  return vec3(0.0);
#endif
}

vec3 worldToTangent(vec3 viewDir, vec2 texCoord) {
#if !DEPTH_PASS
#  ifdef HAS_NORMAL
#    ifdef HAS_TANGENTS
  return normalize(transpose(fs_in.TBN) * viewDir);
#    else
  const mat3 TBN = cotangentFrame(getVertexNormal(), texCoord);
  return normalize(transpose(TBN) * viewDir);
#    endif
#  else
  return vec3(0.0);
#  endif
#else
  return vec3(0.0);
#endif
}

vec2 parallaxOcclusionMapping(sampler2D heightMap, vec2 texCoord, float scale) {
#if !DEPTH_PASS
  const float minLayers = 8.0;
  const float maxLayers = 32.0;

  const vec3 viewDir = worldToTangent(getViewDir(), texCoord);

  const float numLayers =
    mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
  const float layerDepth = 1.0 / numLayers;
  float currentLayerDepth = 0.0;
  const vec2 P = viewDir.xy / viewDir.z * scale;
  const vec2 deltaTexCoords = P / numLayers;

  vec2 currentTexCoords = texCoord;
  float currentDepthMapValue = textureLod(heightMap, currentTexCoords, 0.0).r;

  while (currentLayerDepth < currentDepthMapValue) {
    currentTexCoords -= deltaTexCoords;
    currentDepthMapValue = texture(heightMap, currentTexCoords).r;
    currentLayerDepth += layerDepth;
  }

  const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

  const float afterDepth = currentDepthMapValue - currentLayerDepth;
  const float beforeDepth = textureLod(heightMap, prevTexCoords, 0.0).r -
                            currentLayerDepth + layerDepth;

  const float weight = afterDepth / (afterDepth - beforeDepth);
  return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
#else
  return texCoord;
#endif
}

#endif
