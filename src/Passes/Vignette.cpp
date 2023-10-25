#include "Vignette.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

VignettePass::VignettePass(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("Vignette.frag"));

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
VignettePass::~VignettePass() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource VignettePass::addPass(FrameGraph &fg,
                                         FrameGraphResource input) {
  const auto extent = fg.getDescriptor<FrameGraphTexture>(input).extent;

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "Vignette",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      data.output = builder.create<FrameGraphTexture>(
        "SceneColor", {.extent = extent, .format = PixelFormat::RGB8_UNorm});
      data.output = builder.write(data.output);
    },
    [=, this](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("Vignette");
      TracyGpuZone("Vignette");

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
