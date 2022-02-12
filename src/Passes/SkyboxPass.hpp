#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;
class FrameGraphBlackboard;

class SkyboxPass {
public:
  explicit SkyboxPass(RenderContext &);
  ~SkyboxPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &, FrameGraphBlackboard &,
                                           FrameGraphResource target,
                                           Texture *cubemap);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
