#include "ShadowCascadesBuilder.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <array>

// https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/

namespace {

using Splits = std::vector<float>;
using FrustumCorners = std::array<glm::vec3, 8>;

[[nodiscard]] auto buildCascadeSplits(uint32_t numCascades, float lambda,
                                      float near, float clipRange) {
  constexpr auto kMinDistance = 0.0f, kMaxDistance = 1.0f;

  const auto minZ = near + kMinDistance * clipRange;
  const auto maxZ = near + kMaxDistance * clipRange;

  const auto range = maxZ - minZ;
  const auto ratio = maxZ / minZ;

  Splits cascadeSplits(numCascades);
  // Calculate split depths based on view camera frustum
  // Based on method presented in:
  // https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
  for (uint32_t i{0}; i < cascadeSplits.size(); ++i) {
    const auto p = static_cast<float>(i + 1) / cascadeSplits.size();
    const auto log = minZ * std::pow(ratio, p);
    const auto uniform = minZ + range * p;
    const auto d = lambda * (log - uniform) + uniform;
    cascadeSplits[i] = (d - near) / clipRange;
  }
  return cascadeSplits;
}

[[nodiscard]] auto buildFrustumCorners(const glm::mat4 &inversedViewProj,
                                       float splitDist, float lastSplitDist) {
  // clang-format off
  FrustumCorners frustumCorners{
    // Near
		glm::vec3{-1.0f,  1.0f, -1.0f}, // TL
		glm::vec3{ 1.0f,  1.0f, -1.0f}, // TR
		glm::vec3{ 1.0f, -1.0f, -1.0f}, // BR
		glm::vec3{-1.0f, -1.0f, -1.0f}, // BL
		// Far
    glm::vec3{-1.0f,  1.0f,  1.0f}, // TL
		glm::vec3{ 1.0f,  1.0f,  1.0f}, // TR
		glm::vec3{ 1.0f, -1.0f,  1.0f}, // BR
		glm::vec3{-1.0f, -1.0f,  1.0f}  // BL
  };
  // clang-format on

  // Project frustum corners into world space
  for (auto &p : frustumCorners) {
    const auto temp = inversedViewProj * glm::vec4{p, 1.0f};
    p = glm::vec3{temp} / temp.w;
  }
  for (uint32_t i{0}; i < 4; ++i) {
    const auto cornerRay = frustumCorners[i + 4] - frustumCorners[i];
    const auto nearCornerRay = cornerRay * lastSplitDist;
    const auto farCornerRay = cornerRay * splitDist;
    frustumCorners[i + 4] = frustumCorners[i] + farCornerRay;
    frustumCorners[i] = frustumCorners[i] + nearCornerRay;
  }
  return frustumCorners;
}
[[nodiscard]] auto measureFrustum(const FrustumCorners &frustumCorners) {
  glm::vec3 center{0.0f};
  for (const auto &p : frustumCorners)
    center += p;
  center /= frustumCorners.size();

  auto radius = 0.0f;
  for (const auto &p : frustumCorners) {
    const auto distance = glm::length(p - center);
    radius = glm::max(radius, distance);
  }
  radius = glm::ceil(radius * 16.0f) / 16.0f;
  return std::make_tuple(center, radius);
}

void eliminateShimmering(glm::mat4 &projection, const glm::mat4 &view,
                         uint32_t shadowMapSize) {
  auto shadowOrigin = projection * view * glm::vec4{glm::vec3{0.0f}, 1.0f};
  shadowOrigin *= (static_cast<float>(shadowMapSize) / 2.0f);

  const auto roundedOrigin = glm::round(shadowOrigin);
  auto roundOffset = roundedOrigin - shadowOrigin;
  roundOffset = roundOffset * 2.0f / static_cast<float>(shadowMapSize);
  roundOffset.z = 0.0f;
  roundOffset.w = 0.0f;
  projection[3] += roundOffset;
}

[[nodiscard]] auto buildDirLightMatrix(const glm::mat4 &inversedViewProj,
                                       const glm::vec3 &lightDirection,
                                       uint32_t shadowMapSize, float splitDist,
                                       float lastSplitDist) {
  const auto frustumCorners =
    buildFrustumCorners(inversedViewProj, splitDist, lastSplitDist);
  const auto [center, radius] = measureFrustum(frustumCorners);

  const auto maxExtents = glm::vec3{radius};
  const auto minExtents = -maxExtents;

  const auto eye = center - glm::normalize(lightDirection) * -minExtents.z;
  const auto view = glm::lookAt(eye, center, {0.0f, 1.0f, 0.0f});
  auto projection = glm::ortho(minExtents.x, maxExtents.x, minExtents.y,
                               maxExtents.y, 0.0f, (maxExtents - minExtents).z);
  eliminateShimmering(projection, view, shadowMapSize);
  return projection * view;
}

} // namespace

std::vector<Cascade> buildCascades(const PerspectiveCamera &camera,
                                   const glm::vec3 &lightDirection,
                                   uint32_t numCascades, float lambda,
                                   uint32_t shadowMapSize) {
  const auto clipRange = camera.getFar() - camera.getNear();
  const auto cascadeSplits =
    buildCascadeSplits(numCascades, lambda, camera.getNear(), clipRange);
  const auto inversedViewProj = glm::inverse(camera.getViewProjection());

  auto lastSplitDist = 0.0f;
  std::vector<Cascade> cascades(numCascades);
  for (uint32_t i{0}; i < cascades.size(); ++i) {
    const auto splitDist = cascadeSplits[i];
    cascades[i] = {
      .splitDepth = (camera.getNear() + splitDist * clipRange) * -1.0f,
      .viewProjMatrix =
        buildDirLightMatrix(inversedViewProj, lightDirection, shadowMapSize,
                            splitDist, lastSplitDist),
    };
    lastSplitDist = splitDist;
  }
  return cascades;
}
