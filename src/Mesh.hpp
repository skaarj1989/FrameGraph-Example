#pragma once

#include "RenderContext.hpp"
#include "Material.hpp"
#include "VertexFormat.hpp"
#include "AABB.hpp"

struct SubMesh {
  GeometryInfo geometryInfo;
  std::shared_ptr<Material> material;
};

struct Mesh {
  std::shared_ptr<VertexFormat> vertexFormat;
  std::shared_ptr<VertexBuffer> vertexBuffer;
  std::shared_ptr<IndexBuffer> indexBuffer;

  std::vector<SubMesh> subMeshes;

  AABB aabb{
    .min = glm::vec3{std::numeric_limits<float>::max()},
    .max = glm::vec3{std::numeric_limits<float>::min()},
  };
};
