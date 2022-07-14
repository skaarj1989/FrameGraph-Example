#pragma once

#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"

[[nodiscard]] inline constexpr bool isPowerOf2(uint32_t v) {
  return v && !(v & (v - 1));
}
[[nodiscard]] inline constexpr uint64_t nextPowerOf2(uint64_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

[[nodiscard]] inline constexpr float max3(const glm::vec3 &v) {
  return glm::max(glm::max(v.x, v.y), v.z);
}
