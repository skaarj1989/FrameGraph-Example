#include "FrameGraphBuffer.hpp"
#include "TransientResources.hpp"
#include <format>

void FrameGraphBuffer::create(const Desc &desc, void *allocator) {
  buffer = static_cast<TransientResources *>(allocator)->acquireBuffer(desc);
}
void FrameGraphBuffer::destroy(const Desc &desc, void *allocator) {
  static_cast<TransientResources *>(allocator)->releaseBuffer(
    desc, std::move(buffer));
}

std::string FrameGraphBuffer::toString(const Desc &desc) {
  return std::format("size: {} bytes", desc.size);
}
