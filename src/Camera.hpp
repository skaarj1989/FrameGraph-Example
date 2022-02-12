#pragma once

#include "glm/glm.hpp"

struct Camera {
  glm::mat4 viewMatrix{1.0f};
  glm::mat4 projectionMatrix{1.0f};
};
