#pragma once

#include "fg/FrameGraphResource.hpp"

struct GlobalLightProbeData {
  FrameGraphResource diffuse;  // irradiance
  FrameGraphResource specular; // prefiltered environment map
};
