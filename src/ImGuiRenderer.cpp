#include "ImGuiRenderer.hpp"
#include "ShaderCodeBuilder.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"

#include "tracy/TracyOpenGL.hpp"

#include <span>

namespace {

// Lower-left is default for OpenGL
constexpr auto kClipOriginLowerLeft = true;

[[nodiscard]] glm::mat4 buildProjectionMatrix(const ImVec2 displayPos,
                                              const ImVec2 displaySize) {
  const auto L = displayPos.x;
  const auto R = displayPos.x + displaySize.x;
  auto B = displayPos.y + displaySize.y;
  auto T = displayPos.y;
  if constexpr (!kClipOriginLowerLeft) std::swap(T, B);

  // clang-format off
  return {
    { 2.0f/(R-L),   0.0f,         0.0f,  0.0f },
    { 0.0f,         2.0f/(T-B),   0.0f,  0.0f },
    { 0.0f,         0.0f,        -1.0f,  0.0f },
    { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,  1.0f },
  };
  // clang-format on
}

} // namespace

//
// ImGuiRenderer class:
//

ImGuiRenderer::ImGuiRenderer(RenderContext &rc, ImFontAtlas &fonts)
    : m_renderContext{rc} {
  _setupBuffers();
  _setupFont(fonts);
  _setupPipeline();
}
ImGuiRenderer::~ImGuiRenderer() {
  m_renderContext.destroy(m_uiPipeline)
    .destroy(m_vertexBuffer)
    .destroy(m_indexBuffer)
    .destroy(m_font);
}

void ImGuiRenderer::draw(const ImDrawData *drawData) {
  NAMED_DEBUG_MARKER("GUI");
  TracyGpuZone("GUI");

  // Scale coordinates for retina displays
  // (screen coordinates != framebuffer coordinates)
  const auto framebufferWidth =
    drawData->DisplaySize.x * drawData->FramebufferScale.x;
  const auto framebufferHeight =
    drawData->DisplaySize.y * drawData->FramebufferScale.y;
  // Avoid rendering when minimized
  if (framebufferWidth <= 0 || framebufferHeight <= 0) return;

  _setupRenderState(drawData, framebufferWidth, framebufferHeight);

  // Will project scissor/clipping rectangles into framebuffer space
  // (0,0) unless using multi-viewports
  const auto clipOff = drawData->DisplayPos;
  // (1,1) unless using retina display which are often (2,2)
  const auto clipScale = drawData->FramebufferScale;

  if (drawData->TotalVtxCount > 0) _uploadGeometry(drawData);

  uint32_t globalVertexOffset{0}, globalIndexOffset{0};
  for (const auto *cmdList : drawData->CmdLists) {
    for (const auto &drawCmd : cmdList->CmdBuffer) {
      if (drawCmd.UserCallback != nullptr) {
        // User callback, registered via ImDrawList::AddCallback()
        // (ImDrawCallback_ResetRenderState is a special callback value used
        // by the user to request the renderer to reset render state.)
        if (drawCmd.UserCallback == ImDrawCallback_ResetRenderState)
          _setupRenderState(drawData, framebufferWidth, framebufferHeight);
        else
          drawCmd.UserCallback(cmdList, &drawCmd);
      } else {
        // Project scissor/clipping rectangles into framebuffer space
        const glm::vec2 clipMin{
          (drawCmd.ClipRect.x - clipOff.x) * clipScale.x,
          (drawCmd.ClipRect.y - clipOff.y) * clipScale.y,
        };
        const glm::vec2 clipMax{
          (drawCmd.ClipRect.z - clipOff.x) * clipScale.x,
          (drawCmd.ClipRect.w - clipOff.y) * clipScale.y,
        };
        m_renderContext.setScissor({
          .offset =
            {
              .x = static_cast<int32_t>(clipMin.x),
              .y = static_cast<int32_t>(framebufferHeight - clipMax.y),
            },
          .extent =
            {
              .width = static_cast<uint32_t>(clipMax.x - clipMin.x),
              .height = static_cast<uint32_t>(clipMax.y - clipMin.y),
            },
        });

        const auto texture = reinterpret_cast<Texture *>(drawCmd.TextureId);
        if (texture != &m_font) m_renderContext.bindTexture(0, *texture);

        const auto indexOffset = globalIndexOffset + drawCmd.IdxOffset;
        const auto vertexOffset = globalVertexOffset + drawCmd.VtxOffset;

        m_renderContext.draw(
          m_vertexBuffer, m_indexBuffer,
          {
            .topology = PrimitiveTopology::TriangleList,
            .vertexOffset = globalVertexOffset + drawCmd.VtxOffset,
            .indexOffset = globalIndexOffset + drawCmd.IdxOffset,
            .numIndices = drawCmd.ElemCount,
          });
      }
    }
    globalVertexOffset += cmdList->VtxBuffer.Size;
    globalIndexOffset += cmdList->IdxBuffer.Size;
  }
}

void ImGuiRenderer::_setupBuffers() {
  m_vertexBuffer =
    m_renderContext.createVertexBuffer(sizeof(ImDrawVert), UINT16_MAX);
  m_indexBuffer =
    m_renderContext.createIndexBuffer(IndexType::UInt16, UINT16_MAX);
}
void ImGuiRenderer::_setupFont(ImFontAtlas &fonts) {
  uint8_t *pixels;
  int32_t width, height;
  fonts.GetTexDataAsRGBA32(&pixels, &width, &height);

  m_font = m_renderContext.createTexture2D(
    {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
    PixelFormat::RGBA8_UNorm);
  m_renderContext.setupSampler(m_font, {
                                         .minFilter = TexelFilter::Linear,
                                         .mipmapMode = MipmapMode::None,
                                         .magFilter = TexelFilter::Linear,
                                       });
  m_renderContext.upload(m_font, 0, {width, height},
                         {
                           .format = GL_RGBA,
                           .dataType = GL_UNSIGNED_BYTE,
                           .pixels = pixels,
                         });

  fonts.SetTexID(reinterpret_cast<ImTextureID>(&m_font));
}
void ImGuiRenderer::_setupPipeline() {
  const auto vao = m_renderContext.getVertexArray({
    {0, {VertexAttribute::Type::Float2, 0}},
    {1, {VertexAttribute::Type::Float2, offsetof(ImDrawVert, uv)}},
    {2, {VertexAttribute::Type::UByte4_Norm, offsetof(ImDrawVert, col)}},
  });
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("UI.vert"), shaderCodeBuilder.build("UI.frag"));

  m_uiPipeline = GraphicsPipeline::Builder{}
                   .setDepthStencil({
                     .depthTest = false,
                     .depthWrite = false,
                   })
                   .setRasterizerState({
                     .polygonMode = PolygonMode::Fill,
                     .cullMode = CullMode::None,
                     .scissorTest = true,
                   })
                   .setBlendState(0,
                                  {
                                    .enabled = true,

                                    .srcColor = BlendFactor::SrcAlpha,
                                    .destColor = BlendFactor::OneMinusSrcAlpha,
                                    .colorOp = BlendOp::Add,

                                    .srcAlpha = BlendFactor::One,
                                    .destAlpha = BlendFactor::OneMinusSrcAlpha,
                                    .alphaOp = BlendOp::Add,
                                  })
                   .setVertexArray(vao)
                   .setShaderProgram(program)
                   .build();
}

void ImGuiRenderer::_setupRenderState(const ImDrawData *drawData, float w,
                                      float h) {
  m_renderContext
    .setViewport({.extent =
                    {
                      .width = static_cast<uint32_t>(w),
                      .height = static_cast<uint32_t>(h),
                    }})
    .setGraphicsPipeline(m_uiPipeline)
    .bindTexture(0, m_font)
    .setUniformMat4(
      "u_ProjectionMatrix",
      buildProjectionMatrix(drawData->DisplayPos, drawData->DisplaySize));
}
void ImGuiRenderer::_uploadGeometry(const ImDrawData *drawData) {
  TracyGpuZone("ImGuiRenderer::_uploadGeometry");

  auto vertexOffset = 0, indexOffset = 0;
  for (auto *cmdList : drawData->CmdLists) {
    const auto verticesChunkSize = cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
    m_renderContext.upload(m_vertexBuffer, vertexOffset, verticesChunkSize,
                           cmdList->VtxBuffer.Data);
    vertexOffset += verticesChunkSize;

    const auto indicesChunkSize = cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
    m_renderContext.upload(m_indexBuffer, indexOffset, indicesChunkSize,
                           cmdList->IdxBuffer.Data);
    indexOffset += indicesChunkSize;
  }
}
