#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class DeferredLightingPass {
public:
  DeferredLightingPass(RenderContext &, uint32_t tileSize);
  ~DeferredLightingPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphBlackboard &);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
