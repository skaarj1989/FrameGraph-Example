#pragma once

#include "glm/vec3.hpp"

enum class LightType { Directional, Spot, Point };

struct Light {
  LightType type;
  bool visible{true};
  glm::vec3 position{0.0f};  // in world-space
  glm::vec3 direction{0.0f}; // in world-space
  glm::vec3 color{1.0f};
  float intensity{1.0f};
  float range{1.0f};
  float innerConeAngle{15.0f}; // in degrees
  float outerConeAngle{35.0f}; // in degrees
  bool castsShadow{true};
};
