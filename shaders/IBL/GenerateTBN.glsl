#ifndef _GENERATE_TBN_GLSL_
#define _GENERATE_TBN_GLSL_

mat3 generateTBN(vec3 N) {
  vec3 B = vec3(0.0, 1.0, 0.0);
  float NdotUp = dot(N, vec3(0.0, 1.0, 0.0));
  if (1.0 - abs(NdotUp) <= EPSILON) {
    // Sampling +Y or -Y, so we need a more robust bitangent.
    B = (NdotUp > 0.0) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 0.0, -1.0);
  }
  vec3 T = normalize(cross(B, N));
  B = cross(N, T);
  return mat3(T, B, N);
}

#endif
