#ifndef _RAYCAST_GLSL_
#define _RAYCAST_GLSL_

struct RayCastResult {
  bool hit;
  vec2 uv;
};

// maxHits = 10, maxDist = 200, stepLength = 0.1
RayCastResult rayCast(vec3 rayOrigin, vec3 rayDir, int maxHits, float maxDist,
                      float stepLength) {
  const vec3 stepInc = rayDir * stepLength;
  vec3 currentPos = rayOrigin + stepInc;
  vec3 prevPos = currentPos;

  uint hitCount = 0;
  for (uint i = 0; i < maxDist; ++i) {
    currentPos = prevPos + stepInc;

    const vec2 texCoord = viewToClip(vec4(currentPos, 1.0)).xy * 0.5 + 0.5;
    if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 ||
        texCoord.y > 1.0) {
      break;
    }

    const float depth = getDepth(t_SceneDepth, texCoord);
    const vec3 samplePos = viewPositionFromDepth(depth, texCoord);

    const float deltaDepth = currentPos.z - samplePos.z;
    if (abs(rayDir.z - deltaDepth) < 1.0)
      if (deltaDepth < 0.0) ++hitCount;

    if (hitCount > maxHits) return RayCastResult(true, texCoord);

    prevPos = currentPos;
  }
  return RayCastResult(false, vec2(0.0));
}

#endif
