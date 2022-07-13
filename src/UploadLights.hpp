#pragma once

#include "fg/Fwd.hpp"
#include "Light.hpp"
#include <vector>

void uploadLights(FrameGraph &, FrameGraphBlackboard &,
                  std::vector<const Light *> &&);
