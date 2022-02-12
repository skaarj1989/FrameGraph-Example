#pragma once

#include "glad/gl.h"

class Buffer {
  friend class RenderContext;

public:
  Buffer() = default;
  Buffer(const Buffer &) = delete;
  Buffer(Buffer &&) noexcept;
  virtual ~Buffer();

  Buffer &operator=(const Buffer &) = delete;
  Buffer &operator=(Buffer &&) noexcept;

  explicit operator bool() const;

  [[nodiscard]] GLsizeiptr getSize() const;
  [[nodiscard]] bool isMapped() const;

protected:
  Buffer(GLuint id, GLsizeiptr size);

  operator GLuint() const;

protected:
  GLuint m_id{GL_NONE};
  GLsizeiptr m_size{0};
  void *m_mappedMemory{nullptr};
};
