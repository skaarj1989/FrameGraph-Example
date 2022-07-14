#pragma once

#include "Mesh.hpp"

class BasicShapes {
public:
  explicit BasicShapes(RenderContext &);
  ~BasicShapes();

  const Mesh &getPlane() const;
  const Mesh &getCube() const;
  const Mesh &getSphere() const;

private:
  RenderContext &m_renderContext;

  std::shared_ptr<VertexBuffer> m_vertexBuffer;
  std::shared_ptr<IndexBuffer> m_indexBuffer;

  Mesh m_plane;
  Mesh m_cube;
  Mesh m_sphere;
};
