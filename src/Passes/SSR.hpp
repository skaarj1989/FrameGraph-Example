#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class SSR {
public:
  explicit SSR(RenderContext &);
  ~SSR();

  [[nodiscard]] FrameGraphResource addPass(FrameGraph &,
                                           FrameGraphBlackboard &);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
