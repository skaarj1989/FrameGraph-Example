#include "GammaCorrectionPass.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

GammaCorrectionPass::GammaCorrectionPass(RenderContext &rc)
    : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = rc.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("GammaCorrectionPass.frag"));

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
GammaCorrectionPass::~GammaCorrectionPass() {
  m_renderContext.destroy(m_pipeline);
}

FrameGraphResource GammaCorrectionPass::addPass(FrameGraph &fg,
                                                FrameGraphResource input) {
  const auto extent = fg.getDescriptor<FrameGraphTexture>(input).extent;

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "GammaCorrection",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      data.output = builder.create<FrameGraphTexture>(
        "SceneColor", {.extent = extent, .format = PixelFormat::RGB8_UNorm});
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("GammaCorrection");
      TracyGpuZone("GammaCorrection");

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
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return pass.output;
}
