#pragma once

#include "Buffer.hpp"
#include <string>

class FrameGraphBuffer {
public:
  struct Desc {
    GLsizeiptr size;
  };

  void create(const Desc &, void *allocator);
  void destroy(const Desc &, void *allocator);

  static std::string toString(const Desc &);

  Buffer *buffer{nullptr};
};
