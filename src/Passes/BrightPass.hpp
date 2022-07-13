#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

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
