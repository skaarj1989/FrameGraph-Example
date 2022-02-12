#pragma once

#include "glm/vec3.hpp"

struct Cone {
  glm::vec3 T; // Cone tip
  float h;     // Height of the cone
  glm::vec3 d; // Direction of the cone
  float r;     // Bottom radius of the cone
};
