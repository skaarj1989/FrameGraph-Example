#pragma once

#include "AABB.hpp"

struct Grid {
  explicit Grid(const AABB &);

  AABB aabb;
  glm::uvec3 size;
  float cellSize;
};
