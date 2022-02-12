#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;
class FrameGraphBlackboard;

class DeferredLightingPass {
public:
  DeferredLightingPass(RenderContext &, uint32_t maxNumLights,
                       uint32_t tileSize);
  ~DeferredLightingPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphBlackboard &);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
