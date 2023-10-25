#include "TransparencyCompositionPass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../WeightedBlendedData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

TransparencyCompositionPass::TransparencyCompositionPass(RenderContext &rc)
    : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("TransparencyCompositionPass.frag"));

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
                 .setBlendState(0,
                                {
                                  .enabled = true,
                                  .srcColor = BlendFactor::SrcAlpha,
                                  .destColor = BlendFactor::OneMinusSrcAlpha,
                                  .srcAlpha = BlendFactor::SrcAlpha,
                                  .destAlpha = BlendFactor::OneMinusSrcAlpha,
                                })
                 .setShaderProgram(program)
                 .build();
}
TransparencyCompositionPass::~TransparencyCompositionPass() {
  m_renderContext.destroy(m_pipeline);
}

FrameGraphResource TransparencyCompositionPass::addPass(
  FrameGraph &fg, FrameGraphBlackboard &blackboard, FrameGraphResource target) {
  const auto &weightedBlended = blackboard.get<WeightedBlendedData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(target).extent;

  fg.addCallbackPass(
    "TransparencyComposition",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(weightedBlended.accum);
      builder.read(weightedBlended.reveal);

      target = builder.write(target);
    },
    [=, this](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("TransparencyComposition");
      TracyGpuZone("TrancparencyComposition");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, target),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, weightedBlended.accum))
        .bindTexture(1, getTexture(resources, weightedBlended.reveal))

        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return target;
}
