#pragma once

#include "fg/FrameGraphResource.hpp"

struct ReflectiveShadowMapData {
  FrameGraphResource depth;
  FrameGraphResource position;
  FrameGraphResource normal;
  FrameGraphResource flux;
};
