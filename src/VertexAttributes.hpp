#pragma once

#include <map>

struct VertexAttribute {
  enum class Type {
    Float = 0,
    Float2,
    Float3,
    Float4,

    Int4,

    UByte4_Norm,
  };
  Type type;
  int32_t offset;
};

using VertexAttributes = std::map<int32_t, VertexAttribute>;

namespace std {

template <> struct hash<VertexAttribute> {
  std::size_t operator()(const VertexAttribute &) const noexcept;
};

} // namespace std
