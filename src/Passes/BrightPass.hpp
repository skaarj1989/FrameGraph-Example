#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;

class BrightPass {
public:
  explicit BrightPass(RenderContext &);
  ~BrightPass();

  [[nodiscard]] FrameGraphResource
  addPass(FrameGraph &, FrameGraphResource source, float threshold);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
