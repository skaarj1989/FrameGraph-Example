#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

struct AABB {
  glm::vec3 min, max;

  [[nodiscard]] glm::vec3 getCenter() const;
  [[nodiscard]] float getRadius() const;

  [[nodiscard]] AABB transform(const glm::mat4 &) const;

  auto operator<=>(const AABB &) const = default;
};
