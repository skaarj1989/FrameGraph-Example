#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;

class GammaCorrectionPass {
public:
  explicit GammaCorrectionPass(RenderContext &);
  ~GammaCorrectionPass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphResource input);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
