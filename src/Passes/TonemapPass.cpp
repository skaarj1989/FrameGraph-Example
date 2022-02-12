#include "TonemapPass.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

TonemapPass::TonemapPass(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("TonemapPass.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setDepthStencil({
                   .depthTest = false,
                   .depthWrite = false,
                 })
                 .setRasterizerState({
                   .polygonMode = PolygonMode::Fill,
                   .cullMode = CullMode::Back,
                   .scissorTest = false,
                 })
                 .setShaderProgram(program)
                 .build();
}
TonemapPass::~TonemapPass() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource TonemapPass::addPass(FrameGraph &fg,
                                        FrameGraphResource input,
                                        Tonemap tonemap) {
  const auto extent = fg.getDescriptor<FrameGraphTexture>(input).extent;

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "Tonemapping",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      data.output = builder.create<FrameGraphTexture>(
        "SceneColor", {.extent = extent, .format = PixelFormat::RGB8_UNorm});
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("Tonemapping");
      TracyGpuZone("Tonemapping");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.output),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, input))
        .setUniform1ui("u_Tonemap", static_cast<uint32_t>(tonemap))
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return pass.output;
}
