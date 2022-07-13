#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class Upsampler final {
  friend class BasePass;

public:
  explicit Upsampler(RenderContext &);

  [[nodiscard]] FrameGraphResource
  addPass(FrameGraph &, FrameGraphResource input, float radius);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
