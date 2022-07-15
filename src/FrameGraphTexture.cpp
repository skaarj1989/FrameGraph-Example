#include "FrameGraphTexture.hpp"
#include "TransientResources.hpp"
#include <format>

static std::string toString(Extent2D extent, uint32_t depth) {
  return depth > 0 ? std::format("{}x{}x{}", extent.width, extent.height, depth)
                   : std::format("{}x{}", extent.width, extent.height);
}

void FrameGraphTexture::create(const Desc &desc, void *allocator) {
  texture = static_cast<TransientResources *>(allocator)->acquireTexture(desc);
}
void FrameGraphTexture::destroy(const Desc &desc, void *allocator) {
  static_cast<TransientResources *>(allocator)->releaseTexture(desc, texture);
}

std::string FrameGraphTexture::toString(const Desc &desc) {
  return std::format("{} [{}]", ::toString(desc.extent, desc.depth),
                     ::toString(desc.format));
}
