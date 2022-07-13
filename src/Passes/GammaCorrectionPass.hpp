#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

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
