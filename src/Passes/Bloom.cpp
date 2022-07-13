#include "Bloom.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

Bloom::Bloom(RenderContext &rc)
    : m_renderContext{rc}, m_downsampler{rc}, m_upsampler{rc} {
  ShaderCodeBuilder shaderCodeBuilder;

  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("Bloom.frag"));

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

FrameGraphResource Bloom::resample(FrameGraph &fg, FrameGraphResource input,
                                   float radius) {
  constexpr auto kNumSteps = 6;

  for (auto i = 0; i < kNumSteps; ++i)
    input = m_downsampler.addPass(fg, input, i);
  for (auto i = 0; i < kNumSteps - 1; ++i)
    input = m_upsampler.addPass(fg, input, radius);

  return input;
}

FrameGraphResource Bloom::compose(FrameGraph &fg, FrameGraphResource scene,
                                  FrameGraphResource bloom, float strength) {
  struct Data {
    FrameGraphResource output;
  };
  const auto data = fg.addCallbackPass<Data>(
    "Bloom",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(scene);
      builder.read(bloom);

      const auto &desc = fg.getDescriptor<FrameGraphTexture>(scene);
      data.output = builder.create<FrameGraphTexture>("Scene w/ Bloom",
                                                      {
                                                        .extent = desc.extent,
                                                        .format = desc.format,
                                                      });
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      auto &rc = *static_cast<RenderContext *>(ctx);
      NAMED_DEBUG_MARKER("Bloom");
      TracyGpuZone("Bloom");

      auto &outputTexture = getTexture(resources, data.output);

      const RenderingInfo renderingInfo{
        .area = {.extent = {outputTexture.getExtent()}},
        .colorAttachments = {{
          .image = outputTexture,
          .clearValue = glm::vec4{0.0f},
        }},
      };

      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, scene))
        .bindTexture(1, getTexture(resources, bloom))
        .setUniform1f("u_Strength", strength)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return data.output;
}
