#pragma once

#include "RenderContext.hpp"

class ImFontAtlas;
class ImDrawData;

class ImGuiRenderer {
public:
  ImGuiRenderer(RenderContext &, ImFontAtlas &);
  ~ImGuiRenderer();

  void draw(const ImDrawData *);

private:
  void _setupBuffers();
  void _setupFont(ImFontAtlas &);
  void _setupPipeline();

  void _setupRenderState(const ImDrawData *, float w, float h);
  void _uploadGeometry(const ImDrawData *);

private:
  RenderContext &m_renderContext;

  VertexBuffer m_vertexBuffer;
  IndexBuffer m_indexBuffer;
  Texture m_font;

  GraphicsPipeline m_uiPipeline;
};
