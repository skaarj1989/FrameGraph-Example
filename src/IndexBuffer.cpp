#include "IndexBuffer.hpp"
#include <utility>

IndexType IndexBuffer::getIndexType() const { return m_indexType; }
GLsizeiptr IndexBuffer::getCapacity() const {
  return m_size / static_cast<GLsizei>(m_indexType);
}

IndexBuffer::IndexBuffer(Buffer buffer, IndexType indexType)
    : Buffer{std::move(buffer)}, m_indexType{indexType} {}
