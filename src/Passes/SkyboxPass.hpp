#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

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
