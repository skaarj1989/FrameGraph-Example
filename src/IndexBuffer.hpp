#pragma once

#include "Buffer.hpp"

enum class IndexType { Unknown = 0, UInt8 = 1, UInt16 = 2, UInt32 = 4 };

class IndexBuffer final : public Buffer {
  friend class RenderContext;

public:
  IndexBuffer() = default;

  [[nodiscard]] IndexType getIndexType() const;
  [[nodiscard]] GLsizeiptr getCapacity() const;

private:
  IndexBuffer(Buffer, IndexType);

private:
  IndexType m_indexType{IndexType::Unknown};
};
