#include "FXAA.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../FrameData.hpp"
#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

FXAA::FXAA(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("FXAA.frag"));

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
FXAA::~FXAA() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource FXAA::addPass(FrameGraph &fg,
                                 FrameGraphBlackboard &blackboard,
                                 FrameGraphResource input) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto extent = fg.getDescriptor<FrameGraphTexture>(input).extent;

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "FXAA",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(frameBlock);
      builder.read(input);

      data.output = builder.create<FrameGraphTexture>(
        "AA", {.extent = extent, .format = PixelFormat::RGB8_UNorm});
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("FXAA");
      TracyGpuZone("FXAA");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.output),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))
        .bindTexture(0, getTexture(resources, input))
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return pass.output;
}
