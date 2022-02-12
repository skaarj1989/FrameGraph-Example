#pragma once

#include "Mesh.hpp"
#include "Material.hpp"
#include "AABB.hpp"

struct Renderable {
  const Mesh &mesh;
  const Material &material;
  uint8_t flags{0u};

  glm::mat4 modelMatrix{1.0f};
  AABB aabb;

  auto operator<=>(const Renderable &) const = default;
};

using Renderables = std::vector<Renderable>;

[[nodiscard]] inline bool isTransparent(const Renderable *renderable) {
  return renderable->material.getBlendMode() == BlendMode::Transparent;
}
[[nodiscard]] inline bool isOpaque(const Renderable *renderable) {
  return not isTransparent(renderable);
}
