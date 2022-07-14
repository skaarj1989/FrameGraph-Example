#pragma once

#include "fg/Fwd.hpp"
#include "RenderContext.hpp"

class TiledLighting {
public:
  TiledLighting(RenderContext &, uint32_t tileSize);
  ~TiledLighting();

  void cullLights(FrameGraph &, FrameGraphBlackboard &);

private:
  void _setupPipelines();

  [[nodiscard]] FrameGraphResource _buildFrustums(FrameGraph &,
                                                  FrameGraphBlackboard &,
                                                  uint32_t numFrustums,
                                                  glm::uvec2 numGroups);

  void _cullLights(FrameGraph &, FrameGraphBlackboard &,
                   FrameGraphResource gridFrustums, uint32_t numFrustums,
                   glm::uvec2 numGroups);

private:
  RenderContext &m_renderContext;

  const uint32_t m_tileSize;

  GLuint m_buildFrustumsProgram{GL_NONE};
  GLuint m_cullLightsProgram{GL_NONE};

  GraphicsPipeline m_debugOverlayPipeline;
};
