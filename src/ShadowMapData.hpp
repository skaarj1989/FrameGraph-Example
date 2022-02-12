#pragma once

#include "fg/FrameGraphResource.hpp"

struct ShadowMapData {
  FrameGraphResource cascadedShadowMaps{-1};
  FrameGraphResource viewProjMatrices{-1};
};
