#include "BrightPass.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"
#include "../SceneColorData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

BrightPass::BrightPass(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("BrightPass.frag"));

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
BrightPass::~BrightPass() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource BrightPass::addPass(FrameGraph &fg,
                                       FrameGraphResource source,
                                       float threshold) {
  const auto extent = fg.getDescriptor<FrameGraphTexture>(source).extent;

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "ExtractBrightness",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(source);

      data.output = builder.create<FrameGraphTexture>(
        "BrightColor", {.extent = extent, .format = PixelFormat::RGB16F});
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("ExtractBrightness");
      TracyGpuZone("ExtractBrightness");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.output),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, source))
        .setUniform1f("u_Threshold", threshold)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return pass.output;
}
