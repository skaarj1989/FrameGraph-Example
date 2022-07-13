#include "Blit.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

Blit::Blit(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;

  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("Blit.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setShaderProgram(program)
                 .setDepthStencil({
                   .depthTest = false,
                   .depthWrite = false,
                 })
                 .setRasterizerState({
                   .polygonMode = PolygonMode::Fill,
                   .cullMode = CullMode::Back,
                   .scissorTest = false,
                 })
                 .setBlendState(0,
                                {
                                  .enabled = true,
                                  .srcColor = BlendFactor::One,
                                  .destColor = BlendFactor::One,
                                })
                 .build();
}
Blit::~Blit() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource Blit::addColor(FrameGraph &fg, FrameGraphResource target,
                                  FrameGraphResource source) {
  assert(target != source);

  fg.addCallbackPass(
    "AddColor",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(source);

      target = builder.write(target);
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("AddColor");
      TracyGpuZone("AddColor");

      const auto extent =
        resources.getDescriptor<FrameGraphTexture>(target).extent;
      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, target),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, source))
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return target;
}
