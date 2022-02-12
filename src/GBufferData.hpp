#pragma once

#include "fg/FrameGraphResource.hpp"

struct GBufferData {
  FrameGraphResource depth;
  FrameGraphResource normal;
  FrameGraphResource emissive;
  FrameGraphResource albedo;
  FrameGraphResource metallicRoughnessAO;
};
