
#include "Upsampler.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

Upsampler::Upsampler(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;

  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("Upsample.frag"));

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
                 .build();
}
Upsampler::~Upsampler() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource Upsampler::addPass(FrameGraph &fg, FrameGraphResource input,
                                      float radius) {
  struct Data {
    FrameGraphResource upsampled;
  };
  auto [upsampled] = fg.addCallbackPass<Data>(
    "Upsample",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      const auto &inputDesc = fg.getDescriptor<FrameGraphTexture>(input);
      data.upsampled = builder.create<FrameGraphTexture>(
        "Upsampled", {
                       .extent =
                         {
                           .width = inputDesc.extent.width * 2u,
                           .height = inputDesc.extent.height * 2u,
                         },
                       .format = inputDesc.format,
                     });
      data.upsampled = builder.write(data.upsampled);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      auto &rc = *static_cast<RenderContext *>(ctx);
      NAMED_DEBUG_MARKER("Upsample");
      TracyGpuZone("Upsample");

      auto &targetTexture = getTexture(resources, data.upsampled);

      const RenderingInfo renderingInfo{
        .area = {.extent = {targetTexture.getExtent()}},
        .colorAttachments = {{
          .image = targetTexture,
          .clearValue = glm::vec4{0.0f},
        }},
      };

      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, input))
        .setUniform1f("u_Radius", radius)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return upsampled;
}
