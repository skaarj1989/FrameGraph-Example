#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class FrameGraph;

class VignettePass {
public:
  explicit VignettePass(RenderContext &);
  ~VignettePass();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphResource target);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
