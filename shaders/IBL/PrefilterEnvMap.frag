#version 460 core

// Use with Cube.vert

layout(location = 0) in vec3 v_WorldPos;

layout(binding = 0) uniform samplerCube t_EnvironmentMap;

layout(location = 1) uniform float u_Roughness;

#include <Lib/Hammersley2D.glsl>
#include <Lib/ImportanceSampling.glsl>
#include <Lib/BRDF.glsl>

// https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/

layout(location = 0) out vec3 FragColor;
void main() {
  const float resolution = textureSize(t_EnvironmentMap, 0).x;
  const float omegaP = 4.0 * PI / (6.0 * resolution * resolution);
  const float a = u_Roughness * u_Roughness;

  const vec3 N = normalize(v_WorldPos);
  const vec3 R = N;
  const vec3 V = R;

  float totalWeight = 0.0;
  vec3 color = vec3(0.0);

  const uint kNumSamples = 1024;
  for (uint i = 0; i < kNumSamples; ++i) {
    const vec2 Xi = hammersley2D(i, kNumSamples);
    const vec3 H = importanceSampleGGX(Xi, u_Roughness, N);
    const float VdotH = dot(V, H);
    const vec3 L = 2.0 * VdotH * H - V;

    const float NdotL = clamp01(dot(N, L));
    if (NdotL > 0.0) {
      const float NdotH = clamp01(VdotH);
      const float pdf = D_GGX(NdotH, a) * NdotH / 4.0 + EPSILON;
      const float omegaS = 1.0 / (float(kNumSamples) * pdf);

      const float kMipBias = 1.0;
      const float mipLevel = max(0.5 * log2(omegaS / omegaP) + kMipBias, 0.0);

      color += textureLod(t_EnvironmentMap, L, mipLevel).rgb * NdotL;
      totalWeight += NdotL;
    }
  }
  FragColor = color / totalWeight;
}
