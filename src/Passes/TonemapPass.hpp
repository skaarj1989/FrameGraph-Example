#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

enum class Tonemap : uint32_t { Clamp = 0, ACES, Filmic, Reinhard, Uncharted };

class TonemapPass {
public:
  explicit TonemapPass(RenderContext &);
  ~TonemapPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphResource input, Tonemap);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
