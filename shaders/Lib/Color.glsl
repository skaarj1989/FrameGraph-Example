#ifndef _COLOR_GLSL_
#define _COLOR_GLSL_

const float kGamma = 2.2;
const float kInvGamma = 1.0 / kGamma;

vec3 linearTosRGB(vec3 color) { return pow(color, vec3(kInvGamma)); }
vec4 linearTosRGB(vec4 color) { return vec4(linearTosRGB(color.rgb), color.a); }
vec3 sRGBToLinear(vec3 color) { return vec3(pow(color, vec3(kGamma))); }
vec4 sRGBToLinear(vec4 color) { return vec4(sRGBToLinear(color.rgb), color.a); }

float getLuminance(vec3 color) {
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

#endif
