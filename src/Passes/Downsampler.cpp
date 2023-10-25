#include "Downsampler.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

Downsampler::Downsampler(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;

  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("Downsample.frag"));

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
Downsampler::~Downsampler() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource
Downsampler::addPass(FrameGraph &fg, FrameGraphResource input, uint32_t level) {
  struct Data {
    FrameGraphResource downsampled;
  };
  const auto [downsampled] = fg.addCallbackPass<Data>(
    "Downsample",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      const auto &inputDesc = fg.getDescriptor<FrameGraphTexture>(input);
      data.downsampled = builder.create<FrameGraphTexture>(
        "Downsampled",
        {
          .extent =
            {
              .width =
                glm::max(uint32_t(float(inputDesc.extent.width) * 0.5f), 1u),
              .height =
                glm::max(uint32_t(float(inputDesc.extent.height) * 0.5f), 1u),
            },
          .format = inputDesc.format,
        });
      data.downsampled = builder.write(data.downsampled);
    },
    [=, this](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      auto &rc = *static_cast<RenderContext *>(ctx);
      NAMED_DEBUG_MARKER("Downsample");
      TracyGpuZone("Downsample");

      auto &targetTexture = getTexture(resources, data.downsampled);

      const RenderingInfo renderingInfo{
        .area = {.extent = targetTexture.getExtent()},
        .colorAttachments = {{
          .image = targetTexture,
          .clearValue = glm::vec4{0.0f},
        }},
      };

      // ---

      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, input))
        .setUniform1i("u_MipLevel", level)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return downsampled;
}
