#include "FrameGraphHelper.hpp"
#include "fg/FrameGraph.hpp"
#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"

FrameGraphResource importTexture(FrameGraph &fg, const std::string_view name,
                                 Texture *texture) {
  assert(texture and *texture);
  return fg.import<FrameGraphTexture>(
    name,
    {
      .extent = texture->getExtent(),
      .numMipLevels = texture->getNumMipLevels(),
      .layers = texture->getNumLayers(),
      .format = texture->getPixelFormat(),
    },
    {texture});
}
Texture &getTexture(FrameGraphPassResources &resources, FrameGraphResource id) {
  return *resources.get<FrameGraphTexture>(id).texture;
}

FrameGraphResource importBuffer(FrameGraph &fg, const std::string_view name,
                                Buffer *buffer) {
  assert(buffer and *buffer);
  return fg.import<FrameGraphBuffer>(name, {.size = buffer->getSize()},
                                     {buffer});
}
Buffer &getBuffer(FrameGraphPassResources &resources, FrameGraphResource id) {
  return *resources.get<FrameGraphBuffer>(id).buffer;
}
