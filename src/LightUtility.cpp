#include "LightUtility.hpp"
#include "Math.hpp"

float calculateLightRadius(const glm::vec3 &lightColor) {
  constexpr auto kConstant = 1.0f;
  constexpr auto kLinear = 0.7f;
  constexpr auto kQuadratic = 1.8f;

  return (-kLinear +
          std::sqrt(kLinear * kLinear -
                    4.0f * kQuadratic *
                      (kConstant - (256.0f / 5.0f) * max3(lightColor)))) /
         (2.0f * kQuadratic);
}

Sphere toSphere(const Light &light) {
  assert(light.type == LightType::Point);
  return {.c = light.position, .r = light.range};
}
Cone toCone(const Light &light) {
  assert(light.type == LightType::Spot);

  const auto coneRadius =
    glm::tan(glm::radians(light.outerConeAngle)) * light.range;
  return {
    .T = light.position,
    .h = light.range,
    .d = light.direction,
    .r = coneRadius,
  };
}
