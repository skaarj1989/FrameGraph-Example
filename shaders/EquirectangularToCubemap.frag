#version 460 core

// Use with Cube.vert

layout(location = 0) in vec3 v_WorldPos;

layout(binding = 0) uniform sampler2D t_EquirectangularMap;

layout(location = 1) uniform bool u_InvertY = false;

#include <Lib/Math.glsl>

vec2 sampleSphericalMap(vec3 dir) {
  vec2 v = vec2(atan(dir.z, dir.x), asin(dir.y));
  v *= vec2(1.0 / TWO_PI, 1.0 / PI); // -> [-0.5, 0.5]
  return v + 0.5;                    // -> [0.0, 1.0]
}

layout(location = 0) out vec3 FragColor;
void main() {
  vec2 texCoord = sampleSphericalMap(normalize(v_WorldPos));
  if (u_InvertY) texCoord.y *= -1.0;
  texCoord = clamp01(texCoord);
  const vec3 sampleColor = textureLod(t_EquirectangularMap, texCoord, 0.0).rgb;
  FragColor = sampleColor;
}
