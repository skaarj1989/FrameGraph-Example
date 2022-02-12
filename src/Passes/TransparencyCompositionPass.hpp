#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;
class FrameGraphBlackboard;

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
