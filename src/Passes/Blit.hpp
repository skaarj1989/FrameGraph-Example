#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class Blit {
public:
  explicit Blit(RenderContext &);
  ~Blit();

  [[nodiscard]] FrameGraphResource
  addColor(FrameGraph &, FrameGraphResource target, FrameGraphResource source);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
