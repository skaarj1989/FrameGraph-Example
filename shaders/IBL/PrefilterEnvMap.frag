#version 460 core

// Use with Cube.vert

layout(location = 0) in vec3 v_WorldPos;

layout(binding = 0) uniform samplerCube t_EnvironmentMap;

layout(location = 1) uniform float u_Roughness;

#include <Lib/Math.glsl>
#include <Lib/Hammersley2D.glsl>
#include "GenerateTBN.glsl"

float D_GGX(float NdotH, float roughness) {
  float a = NdotH * roughness;
  float k = roughness / (1.0 - NdotH * NdotH + a * a);
  return k * k * (1.0 / PI);
}

layout(location = 0) out vec3 FragColor;
void main() {
  const float width = textureSize(t_EnvironmentMap, 0).x;
  const float a = u_Roughness * u_Roughness;

  const vec3 N = normalize(v_WorldPos);
  const mat3 TBN = generateTBN(N);

  float weight = 0.0;
  vec3 color = vec3(0.0);

  const uint kNumSamples = 1024;
  for (uint i = 0; i < kNumSamples; ++i) {
    const vec2 Xi = hammersley2D(i, kNumSamples);

    const float cosTheta =
      clamp01(sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y)));
    const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    const float phi = 2.0 * PI * Xi.x;

    const float pdf = D_GGX(cosTheta, a) / 4.0;

    const vec3 localSpaceDirection =
      normalize(vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
    const vec3 H = TBN * localSpaceDirection;

    const vec3 V = N;
    const vec3 L = normalize(reflect(-V, H));
    const float NdotL = dot(N, L);

    if (NdotL > 0.0) {
      float lod = 0.5 * log2(6.0 * float(width) * float(width) /
                             (float(kNumSamples) * pdf));
      if (u_Roughness == 0.0) lod = 0.0;
      const vec3 sampleColor = textureLod(t_EnvironmentMap, L, lod).rgb;
      if (!any(isinf(sampleColor))) {
        // BUG: glGenerateTextureMipmap stores `inf` values
        color += sampleColor * NdotL;
        weight += NdotL;
      }
    }
  }
  FragColor = color / ((weight != 0.0) ? weight : float(kNumSamples));
}
