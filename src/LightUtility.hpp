#pragma once

#include "Light.hpp"
#include "Sphere.hpp"
#include "Cone.hpp"

[[nodiscard]] float calculateLightRadius(const glm::vec3 &lightColor);

[[nodiscard]] Sphere toSphere(const Light &);
[[nodiscard]] Cone toCone(const Light &);
