#pragma once

#include "RenderContext.hpp"

class CubemapConverter {
public:
  explicit CubemapConverter(RenderContext &);
  ~CubemapConverter();

  [[nodiscard]] Texture equirectangularToCubemap(const Texture &);

private:
  RenderContext &m_renderContext;
  GraphicsPipeline m_pipeline;
};
