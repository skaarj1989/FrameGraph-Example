#version 460 core

// Use with Geometry.vert

#include "BasePassAttributes.glsl"

#include <Lib/Math.glsl>
#include <Lib/Color.glsl>
#include <Lib/NormalMapping.glsl>

#include <Material.glsl>

#if BLEND_MODE == BLEND_MODE_OPAQUE
// https://www.khronos.org/opengl/wiki/Early_Fragment_Test
layout(early_fragment_tests) in;
#endif

void main() {
  Material material = _initMaterial();
  _executeUserCode(material);

#if BLEND_MODE == BLEND_MODE_MASKED
  if (!material.visible) discard;
#endif
}
