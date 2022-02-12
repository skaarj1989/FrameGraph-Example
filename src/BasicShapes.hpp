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

  VertexBuffer m_vertexBuffer;
  IndexBuffer m_indexBuffer;

  std::unique_ptr<Mesh> m_plane, m_cube, m_sphere;
};
