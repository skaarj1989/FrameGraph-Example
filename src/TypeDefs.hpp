#pragma once

#include "glad/gl.h"
#include <cstdint>
#include <compare>

struct Offset2D {
  int32_t x{0}, y{0};

  auto operator<=>(const Offset2D &) const = default;
};
struct Extent2D {
  uint32_t width{0}, height{0};

  auto operator<=>(const Extent2D &) const = default;
};
struct Rect2D {
  Offset2D offset;
  Extent2D extent;

  auto operator<=>(const Rect2D &) const = default;
};

enum class CompareOp : GLenum {
  Never = GL_NEVER,
  Less = GL_LESS,
  Equal = GL_EQUAL,
  LessOrEqual = GL_LEQUAL,
  Greater = GL_GREATER,
  NotEqual = GL_NOTEQUAL,
  GreaterOrEqual = GL_GEQUAL,
  Always = GL_ALWAYS
};
