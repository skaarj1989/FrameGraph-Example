#version 460 core

// Use with Cube.vert

layout(location = 0) in vec3 v_WorldPos;

layout(binding = 0) uniform samplerCube t_EnvironmentMap;

#include <Lib/Math.glsl>

layout(location = 0) out vec3 FragColor;
void main() {
  const vec3 N = normalize(v_WorldPos);

  // Tangent space calculation from origin point
  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  const vec3 right = normalize(cross(up, N));
  up = cross(N, right);

  const float kDeltaPhi = TWO_PI / 360.0;
  const float kDeltaTheta = HALF_PI / 90.0;

  uint sampleCount = 0;
  vec3 color = vec3(0.0);
  for (float phi = 0; phi < TWO_PI; phi += kDeltaPhi) {
    for (float theta = 0; theta < HALF_PI; theta += kDeltaTheta) {
      vec3 tempVec = cos(phi) * right + sin(phi) * up;
      vec3 sampleVec = cos(theta) * N + sin(theta) * tempVec;
      color += textureLod(t_EnvironmentMap, sampleVec, 0.0).rgb * cos(theta) *
               sin(theta);
      ++sampleCount;
    }
  }
  FragColor = PI * color / float(sampleCount);
}
