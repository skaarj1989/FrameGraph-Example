#ifndef _BASE_PASS_ATTRIBUTES_GLSL_
#define _BASE_PASS_ATTRIBUTES_GLSL_

layout(location = 0) in VertexData {
#if !DEPTH_PASS
  vec4 fragPosViewSpace;
#endif
#ifdef HAS_NORMAL
#  ifdef HAS_TANGENTS
  mat3 TBN; // tangent-space -> to view-space
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
fs_in;

// -- FUNCTIONS:

vec3 getViewDir() {
#if !DEPTH_PASS
  return normalize(-fs_in.fragPosViewSpace.xyz);
#else
  return vec3(0.0);
#endif
}

vec3 getVertexNormal() {
#ifdef HAS_NORMAL
  return normalize(
#  ifdef HAS_TANGENTS
    fs_in.TBN[2]
#  else
    fs_in.normal
#  endif
    * (gl_FrontFacing ? 1.0 : -1.0));
#else
  return vec3(0.0);
#endif
}

vec3 getVertexColor() {
#ifdef HAS_COLOR
  fs_in.color;
#else
  return vec3(1.0);
#endif
}

vec2 getTexCoord0() {
#ifdef HAS_TEXCOORD0
  return fs_in.texCoord0;
#else
  return vec2(0.0);
#endif
}

vec2 getTexCoord1() {
#ifdef HAS_TEXCOORD1
  return fs_in.texCoord1;
#else
  return vec2(0.0);
#endif
}

#endif
