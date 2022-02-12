#pragma once

#include "Buffer.hpp"

class VertexBuffer final : public Buffer {
  friend class RenderContext;

public:
  VertexBuffer() = default;

  [[nodiscard]] GLsizei getStride() const;
  [[nodiscard]] GLsizeiptr getCapacity() const;

private:
  VertexBuffer(Buffer, GLsizei stride);

private:
  GLsizei m_stride{0};
};
