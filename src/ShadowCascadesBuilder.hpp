#pragma once

#include "PerspectiveCamera.hpp"
#include <vector>

struct Cascade {
  float splitDepth;
  glm::mat4 viewProjMatrix;
};

[[nodiscard]] std::vector<Cascade>
buildCascades(const PerspectiveCamera &, const glm::vec3 &lightDirection,
              uint32_t numCascades, float lambda, uint32_t shadowMapSize);
