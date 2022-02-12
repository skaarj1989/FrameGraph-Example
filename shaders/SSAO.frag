#version 460 core

layout(location = 0) in vec2 v_TexCoord;

#include <Resources/FrameBlock.glsl>

layout(binding = 0) uniform sampler2D t_SceneDepth;

layout(binding = 1) uniform sampler2D t_GBuffer0; // .rgb = Normal
layout(binding = 2) uniform sampler2D t_Noise;

layout(binding = 1, std140) uniform KernelBlock { vec4 samples[KERNEL_SIZE]; }
u_Kernel;

#include <Lib/Depth.glsl>

const float kRadius = 0.5;
const float kBias = 0.025;

layout(location = 0) out float FragColor;
void main() {
  const float depth = getDepth(t_SceneDepth, v_TexCoord);
  if (depth >= 1.0) {
    discard;
    return;
  }

  const vec2 gbufferSize = textureSize(t_SceneDepth, 0);
  const vec2 noiseSize = textureSize(t_Noise, 0);
  const vec2 noiseTexCoord = (gbufferSize / noiseSize) * v_TexCoord;
  const vec3 rvec = texture(t_Noise, noiseTexCoord).xyz;

  const vec3 N = normalize(texture(t_GBuffer0, v_TexCoord).rgb);
  const vec3 T = normalize(rvec - N * dot(rvec, N));
  const vec3 B = cross(N, T);
  const mat3 TBN = mat3(T, B, N); // tangent-space -> view-space

  const vec3 fragPosViewSpace = viewPositionFromDepth(depth, v_TexCoord);

  float occlusion = 0.0;
  for (uint i = 0; i < KERNEL_SIZE; ++i) {
    vec3 samplePos = TBN * u_Kernel.samples[i].xyz;
    samplePos = fragPosViewSpace + samplePos * kRadius;
    const vec2 offset = viewToClip(vec4(samplePos, 1.0)).xy * 0.5 + 0.5;
    float sampleDepth = getDepth(t_SceneDepth, offset);
    sampleDepth = viewPositionFromDepth(sampleDepth, offset).z;

    const float rangeCheck =
      smoothstep(0.0, 1.0, kRadius / abs(fragPosViewSpace.z - sampleDepth));
    occlusion += (sampleDepth >= samplePos.z + kBias ? 1.0 : 0.0) * rangeCheck;
  }

  const float kInvKernelSize = 1.0 / KERNEL_SIZE;
  FragColor = 1.0 - occlusion * kInvKernelSize;
}
