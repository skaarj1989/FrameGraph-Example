#ifndef _IMPORTANCE_SAMPLE_GLSL_
#define _IMPORTANCE_SAMPLE_GLSL_

#include <Lib/Math.glsl>

vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 N) {
  const float a = roughness * roughness;
  const float phi = 2.0 * PI * Xi.x + random(N.xz) * 0.1;
  const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  // From spherical coordinates to cartesian coordinates
  const vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

  // From tangent-space H vector to world-space sample vector
  const vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  const vec3 tangent = normalize(cross(up, N));
  const vec3 bitangent = cross(N, tangent);
  // Tangent to world space
  return tangent * H.x + bitangent * H.y + N * H.z;
}

#endif
