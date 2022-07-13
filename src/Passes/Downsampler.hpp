#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class Downsampler {
public:
  explicit Downsampler(RenderContext &);
  ~Downsampler();

  [[nodiscard]] FrameGraphResource
  addPass(FrameGraph &, FrameGraphResource input, uint32_t level);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
