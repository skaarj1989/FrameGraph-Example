#pragma once

#include "Texture.hpp"
#include <string>

class FrameGraphTexture {
public:
  struct Desc {
    Extent2D extent;
    uint32_t depth{0};
    uint32_t numMipLevels{1};
    uint32_t layers{0};
    PixelFormat format{PixelFormat::Unknown};
    bool shadowSampler{false};
  };

  void create(const Desc &, void *allocator);
  void destroy(const Desc &, void *allocator);

  static std::string toString(const Desc &);

  Texture *texture{nullptr};
};
