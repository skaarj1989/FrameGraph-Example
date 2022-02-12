#pragma once

#include "fg/FrameGraphResource.hpp"
#include <optional>

struct LightCullingData {
  FrameGraphResource lightIndices;
  FrameGraphResource lightGrid;
  std::optional<FrameGraphResource> debugMap;
};
