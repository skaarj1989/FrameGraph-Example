#include "SSR.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../FrameData.hpp"
#include "../GBufferData.hpp"
#include "../SceneColorData.hpp"
#include "../ReflectionsData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

SSR::SSR(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("SSR.frag"));

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
SSR::~SSR() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource SSR::addPass(FrameGraph &fg,
                                FrameGraphBlackboard &blackboard) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;
  const auto &sceneColor = blackboard.get<SceneColorData>();

  auto &pass = fg.addCallbackPass<ReflectionsData>(
    "SSR",
    [&](FrameGraph::Builder &builder, ReflectionsData &data) {
      builder.read(frameBlock);

      builder.read(gBuffer.depth);
      builder.read(gBuffer.normal);
      builder.read(gBuffer.metallicRoughnessAO);
      builder.read(sceneColor.hdr);

      data.reflections = builder.create<FrameGraphTexture>(
        "Reflections", {.extent = extent, .format = PixelFormat::RGB16F});
      data.reflections = builder.write(data.reflections);
    },
    [=](const ReflectionsData &data, FrameGraphPassResources &resources,
        void *ctx) {
      NAMED_DEBUG_MARKER("SSR");
      TracyGpuZone("SSR");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.reflections),
          .clearValue = glm::vec4{0.0f},
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))

        .bindTexture(0, getTexture(resources, gBuffer.depth))
        .bindTexture(1, getTexture(resources, gBuffer.normal))
        .bindTexture(2, getTexture(resources, gBuffer.metallicRoughnessAO))
        .bindTexture(3, getTexture(resources, sceneColor.hdr))

        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  blackboard.add<ReflectionsData>(pass);

  return pass.reflections;
}
