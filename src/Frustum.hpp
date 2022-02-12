#pragma once

#include "Plane.hpp"
#include "AABB.hpp"
#include "Sphere.hpp"
#include "Cone.hpp"
#include <array>

class Frustum final {
public:
  Frustum() = default;
  explicit Frustum(const glm::mat4 &);
  Frustum(const Frustum &) = delete;
  Frustum(Frustum &&) noexcept = default;
  ~Frustum() = default;

  Frustum &operator=(const Frustum &) = delete;
  Frustum &operator=(Frustum &&) noexcept = default;

  /**
   * @param [in] m
   *  (if) projection    => planes in view-space
   *  (if) viewProj      => planes in world-space
   *  (if) modelViewProj => planes in model-space
   */
  void update(const glm::mat4 &m);

  [[nodiscard]] bool testPoint(const glm::vec3 &) const;
  [[nodiscard]] bool testAABB(const AABB &) const;
  [[nodiscard]] bool testSphere(const Sphere &) const;
  [[nodiscard]] bool testCone(const Cone &) const;

private:
  std::array<Plane, 6> m_planes;
};
