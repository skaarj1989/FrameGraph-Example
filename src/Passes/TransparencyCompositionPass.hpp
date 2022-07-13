#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class TransparencyCompositionPass {
public:
  explicit TransparencyCompositionPass(RenderContext &);
  ~TransparencyCompositionPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &, FrameGraphBlackboard &,
                                           FrameGraphResource target);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
