#include "Buffer.hpp"
#include "spdlog/spdlog.h"

Buffer::Buffer(Buffer &&other) noexcept
    : m_id{other.m_id}, m_size{other.m_size}, m_mappedMemory{
                                                other.m_mappedMemory} {
  memset(&other, 0, sizeof(Buffer));
}
Buffer::~Buffer() {
  if (m_id != GL_NONE) SPDLOG_WARN("Texture leak: {}", m_id);
}
Buffer &Buffer::operator=(Buffer &&rhs) noexcept {
  if (this != &rhs) {
    memcpy(this, &rhs, sizeof(Buffer));
    memset(&rhs, 0, sizeof(Buffer));
  }
  return *this;
}

Buffer::operator bool() const { return m_id != GL_NONE; }

GLsizeiptr Buffer::getSize() const { return m_size; }
bool Buffer::isMapped() const { return m_mappedMemory != nullptr; }

Buffer::Buffer(GLuint id, GLsizeiptr size) : m_id{id}, m_size{size} {}

Buffer::operator GLuint() const { return m_id; }
