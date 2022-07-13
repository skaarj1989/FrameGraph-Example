#pragma once

#include "Downsampler.hpp"
#include "Upsampler.hpp"

class Bloom {
public:
  explicit Bloom(RenderContext &);

  [[nodiscard]] FrameGraphResource
  resample(FrameGraph &, FrameGraphResource input, float radius);

  [[nodiscard]] FrameGraphResource compose(FrameGraph &,
                                           FrameGraphResource scene,
                                           FrameGraphResource bloom,
                                           float strength);

private:
  RenderContext &m_renderContext;
  Downsampler m_downsampler;
  Upsampler m_upsampler;

  GraphicsPipeline m_pipeline;
};
