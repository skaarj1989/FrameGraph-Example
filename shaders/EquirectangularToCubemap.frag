#version 460 core

// Use with Cube.vert

layout(location = 0) in vec3 v_WorldPos;

layout(binding = 0) uniform sampler2D t_EquirectangularMap;

layout(location = 1) uniform bool u_InvertY = false;

vec2 sampleSphericalMap(vec3 v) {
  const vec2 texCoord = vec2(atan(v.z, v.x), asin(v.y));
  return vec2(0.1591, 0.3183) * texCoord + 0.5;
}

layout(location = 0) out vec3 FragColor;
void main() {
  vec2 texCoord = sampleSphericalMap(normalize(v_WorldPos));
  if (u_InvertY) texCoord.y *= -1.0;
  FragColor = texture(t_EquirectangularMap, texCoord).rgb;
}
