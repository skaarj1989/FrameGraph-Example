#version 460 core

// VertexFormat.hpp

layout(location = 0) in vec3 a_Position;
#ifdef HAS_COLOR
layout(location = 1) in vec3 a_Color0;
#endif
#ifdef HAS_NORMAL
layout(location = 2) in vec3 a_Normal;
#endif
#ifdef HAS_TEXCOORD0
layout(location = 3) in vec2 a_TexCoord0;
#  ifdef HAS_TANGENTS
layout(location = 5) in vec3 a_Tangent;
layout(location = 6) in vec3 a_Bitangent;
#  endif
#endif
#ifdef HAS_TEXCOORD1
layout(location = 4) in vec2 a_TexCoord1;
#endif
#ifdef IS_SKINNED
layout(location = 7) in ivec4 a_Joints;
layout(location = 8) in vec4 a_Weights;
#endif

struct Transform {
  mat4 modelView;
  mat4 normalMatrix;
  mat4 modelViewProj;
};
layout(location = 0) uniform Transform u_Transform;

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out VertexData {
#if !DEPTH_PASS
  vec4 fragPosViewSpace;
#endif
#ifdef HAS_NORMAL
#  ifdef HAS_TANGENTS
  mat3 TBN; // tangent-space -> view-space
#  else
  vec3 normal;
#  endif
#endif
#ifdef HAS_TEXCOORD0
  vec2 texCoord0;
#endif
#ifdef HAS_TEXCOORD1
  vec2 texCoord1;
#endif
#ifdef HAS_COLOR
  vec3 color;
#endif
}
vs_out;

void main() {
  vec3 vertexOffset = vec3(0.0);

  const mat4 skinMatrix = mat4(1.0);
#ifdef IS_SKINNED
  skinMatrix = getSkinMatrix(instanceInfo.skinOffset);
#endif
  const vec4 localPos = skinMatrix * vec4(a_Position + vertexOffset, 1.0);

#if !DEPTH_PASS
  vs_out.fragPosViewSpace = u_Transform.modelView * localPos;

#  ifdef HAS_NORMAL
  const mat3 normalMatrix = mat3(u_Transform.normalMatrix * skinMatrix);
  const vec3 N = normalize(normalMatrix * a_Normal);
#    ifdef HAS_TANGENTS
  vec3 T = normalize(normalMatrix * a_Tangent);
  T = normalize(T - dot(T, N) * N);
  vec3 B = normalize(normalMatrix * a_Bitangent);
  vs_out.TBN = mat3(T, B, N);
#    else
  vs_out.normal = N;
#    endif
#  endif
#endif

#ifdef HAS_TEXCOORD0
  vs_out.texCoord0 = a_TexCoord0;
#endif
#ifdef HAS_TEXCOORD1
  vs_out.texCoord1 = a_TexCoord1;
#endif

#ifdef HAS_COLOR
  vs_out.color = a_Color0;
#endif

  gl_Position = u_Transform.modelViewProj * localPos;
}
