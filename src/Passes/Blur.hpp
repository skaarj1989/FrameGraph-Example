#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class Blur {
public:
  explicit Blur(RenderContext &);
  ~Blur();

  [[nodiscard]] FrameGraphResource
  addTwoPassGaussianBlur(FrameGraph &, FrameGraphResource input, float scale);

private:
  [[nodiscard]] FrameGraphResource _addPass(FrameGraph &,
                                            FrameGraphResource input,
                                            float scale, bool horizontal);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
