#pragma once

#include "RenderContext.hpp"
#include "VertexFormat.hpp"
#include "AABB.hpp"

struct Mesh {
  std::shared_ptr<VertexFormat> vertexFormat;
  std::reference_wrapper<const VertexBuffer> vertexBuffer;
  OptionalReference<const IndexBuffer> indexBuffer;
  GeometryInfo geometryInfo;
  AABB aabb;
};
