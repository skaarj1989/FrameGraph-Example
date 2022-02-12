#pragma once

#include "fg/FrameGraphResource.hpp"
#include "../RenderContext.hpp"

class FrameGraph;
class FrameGraphBlackboard;

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
