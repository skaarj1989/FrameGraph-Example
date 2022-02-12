#include "FrameGraphTexture.hpp"
#include "TransientResources.hpp"
#include <format>

void FrameGraphTexture::create(const Desc &desc, void *allocator) {
  texture = static_cast<TransientResources *>(allocator)->acquireTexture(desc);
}
void FrameGraphTexture::destroy(const Desc &desc, void *allocator) {
  static_cast<TransientResources *>(allocator)->releaseTexture(desc, texture);
}

std::string FrameGraphTexture::toString(const Desc &desc) {
  return std::format("{}x{} [{}]", desc.extent.width, desc.extent.height,
                     ::toString(desc.format));
}
