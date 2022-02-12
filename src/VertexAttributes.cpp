#include "VertexAttributes.hpp"
#include "Hash.hpp"

std::size_t std::hash<VertexAttribute>::operator()(
  const VertexAttribute &attribute) const noexcept {
  std::size_t h{0};
  hashCombine(h, attribute.type, attribute.offset);
  return h;
}
