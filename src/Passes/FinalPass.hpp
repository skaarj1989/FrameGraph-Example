#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

enum class OutputMode : uint32_t {
  Depth = 0,
  Emissive,
  BaseColor,
  Normal,
  Metallic,
  Roughness,
  AmbientOcclusion,

  SSAO,
  BrightColor,
  Reflections,

  Accum,
  Reveal,

  LightHeatmap,

  HDR,
  FinalImage,
};

class FinalPass {
public:
  explicit FinalPass(RenderContext &);
  ~FinalPass();

  void compose(FrameGraph &, const FrameGraphBlackboard &, OutputMode);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
