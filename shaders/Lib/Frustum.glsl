#ifndef _FRUSTUM_GLSL_
#define _FRUSTUM_GLSL_

#include <Lib/Plane.glsl>

struct Frustum {
  Plane planes[4];
};

#define _DECLARE_FRUSTUMS(index, access, name)                                 \
  layout(binding = index, std430) access buffer GridFrustums { Frustum name[]; }

#endif
