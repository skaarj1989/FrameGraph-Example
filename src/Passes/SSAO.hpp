#pragma once

#include "fg/Fwd.hpp"
#include "../RenderContext.hpp"

class SSAO {
public:
  explicit SSAO(RenderContext &, uint32_t kernelSize = 32u);
  ~SSAO();

  void addPass(FrameGraph &, FrameGraphBlackboard &);

private:
  void _generateNoiseTexture();
  void _generateSampleKernel(uint32_t kernelSize);
  void _createPipeline(uint32_t kernelSize);

private:
  RenderContext &m_renderContext;

  Texture m_noise;
  UniformBuffer m_kernelBuffer;
  GraphicsPipeline m_pipeline;
};
