#pragma once

#include "fg/Fwd.hpp"
#include "PerspectiveCamera.hpp"
#include "RenderContext.hpp"

struct FrameInfo {
  float time;
  float deltaTime;
  Extent2D resolution;
  const PerspectiveCamera &camera;
  uint32_t features;
  uint32_t debugFlags;
};
void uploadFrameBlock(FrameGraph &, FrameGraphBlackboard &, const FrameInfo &);
