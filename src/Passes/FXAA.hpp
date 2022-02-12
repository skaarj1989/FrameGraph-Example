#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;

class FXAA {
public:
  explicit FXAA(RenderContext &);
  ~FXAA();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphResource input);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
