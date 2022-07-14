#version 460 core

// http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
// http://imanolfotia.com/blog/update/2017/03/11/ScreenSpaceReflections.html
// http://roar11.com/2015/07/screen-space-glossy-reflections/

// https://www.programmersought.com/article/1781886801/
// https://virtexedge.design/shader-series-basic-screen-space-reflections/
// https://thomasdeliot.wixsite.com/blog/single-post/2018/04/26/small-project-opengl-engine-and-pbr-deferred-pipeline-with-ssrssao

// https://sakibsaikia.github.io/graphics/2016/12/26/Screen-Space-Reflection-in-Killing-Floor-2.html

layout(location = 0) in vec2 v_TexCoord;

#include <Resources/FrameBlock.glsl>

layout(binding = 0) uniform sampler2D t_SceneDepth;

layout(binding = 1) uniform sampler2D t_GBuffer0; // .rgb = Normal
// .r = Metallic, .g = Roughness, .b = AO, .a = ShadingModel/MaterialFlags
layout(binding = 2) uniform sampler2D t_GBuffer3;

layout(binding = 3) uniform sampler2D t_SceneColor;

#include <Lib/Depth.glsl>
#include <Lib/BRDF.glsl>
#include <Lib/RayCast.glsl>

#include <MaterialDefines.glsl>

vec3 hash(vec3 a) {
  const vec3 scale = vec3(0.8);
  const float K = 19.19;

  a = fract(a * scale);
  a += dot(a, a.yxz + K);
  return fract((a.xxy + a.yxx) * a.zyx);
}

layout(location = 0) out vec3 FragColor;
void main() {
  const float depth = getDepth(t_SceneDepth, v_TexCoord);
  if (depth >= 1.0) {
    discard;
    return;
  }

  const vec4 temp = texture(t_GBuffer3, v_TexCoord);

  const int encoded = int(temp.a * 255.0);
  const int shadingModel = bitfieldExtract(encoded, 0, 2);

  const float metallic = clamp01(temp.r);
  const float roughness = clamp01(temp.g);

  if (shadingModel == SHADING_MODEL_UNLIT || roughness > 0.9 ||
      metallic < 0.1) {
    discard;
    return;
  }

  const vec3 fragPosViewSpace = viewPositionFromDepth(depth, v_TexCoord);
  vec3 N = normalize(texture(t_GBuffer0, v_TexCoord).xyz);
  N = mat3(u_Frame.camera.view) * N;
  const vec3 V = normalize(-fragPosViewSpace);
  const float NdotV = clamp01(dot(N, V));

  vec3 jitter = hash(fragPosViewSpace) * roughness * 0.1;
  jitter.x = 0.0;

  const vec3 R = normalize(reflect(-V, N));
  RayCastResult rayResult =
    rayCast(fragPosViewSpace, normalize(R + jitter), 10, 200.0, 0.1);
  if (!rayResult.hit) discard;

  const vec3 reflectedColor = texture(t_SceneColor, rayResult.uv).rgb;

  const vec3 albedo = texture(t_SceneColor, v_TexCoord).rgb;
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);
  const vec3 F = F_Schlick(F0, vec3(1.0), NdotV, roughness);
  float visibility = 1.0 - max(dot(V, R), 0.0);

  const vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5) - rayResult.uv));
  const float screenEdgeFactor = clamp01(1.0 - (dCoords.x + dCoords.y));
  const float kReflectionSpecularFalloffExponent = 3.0;
  const float reflectionMultiplier =
    pow(metallic, kReflectionSpecularFalloffExponent) * screenEdgeFactor * -R.z;

  FragColor = reflectedColor * F * visibility * clamp01(reflectionMultiplier);
}
