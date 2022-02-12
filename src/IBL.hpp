#pragma once

#include "RenderContext.hpp"

class IBL {
public:
  explicit IBL(RenderContext &);
  ~IBL();

  [[nodiscard]] Texture generateBRDF();

  [[nodiscard]] Texture generateIrradiance(const Texture &cubemap);
  [[nodiscard]] Texture prefilterEnvMap(const Texture &cubemap);

private:
  RenderContext &m_renderContext;

  GraphicsPipeline m_brdfPipeline;
  GraphicsPipeline m_irradiancePipeline;
  GraphicsPipeline m_prefilterEnvMapPipeline;
};
