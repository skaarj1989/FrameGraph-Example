#pragma once

#include "fg/FrameGraphResource.hpp"
#include <string_view>

class FrameGraph;
class FrameGraphPassResources;

class Texture;

[[nodiscard]] FrameGraphResource
importTexture(FrameGraph &, const std::string_view name, Texture *);
[[nodiscard]] Texture &getTexture(FrameGraphPassResources &,
                                  FrameGraphResource id);

class Buffer;

[[nodiscard]] FrameGraphResource
importBuffer(FrameGraph &, const std::string_view name, Buffer *);

[[nodiscard]] Buffer &getBuffer(FrameGraphPassResources &,
                                FrameGraphResource id);
