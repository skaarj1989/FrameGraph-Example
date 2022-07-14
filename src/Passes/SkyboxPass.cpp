#include "SkyboxPass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../FrameData.hpp"
#include "../GBufferData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

SkyboxPass::SkyboxPass(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("Skybox.vert"),
                             shaderCodeBuilder.build("Skybox.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setDepthStencil({
                   .depthTest = true,
                   .depthWrite = false,
                   .depthCompareOp = CompareOp::Less,
                 })
                 .setRasterizerState({
                   .polygonMode = PolygonMode::Fill,
                   .cullMode = CullMode::Back,
                   .scissorTest = false,
                 })
                 .setShaderProgram(program)
                 .build();
}
SkyboxPass::~SkyboxPass() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource SkyboxPass::addPass(FrameGraph &fg,
                                       FrameGraphBlackboard &blackboard,
                                       FrameGraphResource target,
                                       Texture *cubemap) {
  assert(cubemap && *cubemap);

  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(target).extent;

  const auto skybox = importTexture(fg, "Skybox", cubemap);

  fg.addCallbackPass(
    "Skybox Pass",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(frameBlock);
      builder.read(gBuffer.depth);
      builder.read(skybox);

      target = builder.write(target);
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("DrawSkybox");
      TracyGpuZone("DrawSkybox");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, target),
        }},
        .depthAttachment =
          AttachmentInfo{
            .image = getTexture(resources, gBuffer.depth),
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))
        .bindTexture(0, getTexture(resources, skybox))
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return target;
}
