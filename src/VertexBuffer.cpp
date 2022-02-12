#include "VertexBuffer.hpp"
#include <utility>

GLsizei VertexBuffer::getStride() const { return m_stride; }
GLsizeiptr VertexBuffer::getCapacity() const { return m_size / m_stride; }

VertexBuffer::VertexBuffer(Buffer buffer, GLsizei stride)
    : Buffer{std::move(buffer)}, m_stride{stride} {}
