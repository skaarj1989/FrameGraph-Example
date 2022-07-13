#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class FXAA {
public:
  explicit FXAA(RenderContext &);
  ~FXAA();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &, FrameGraphBlackboard &,
                                           FrameGraphResource input);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
