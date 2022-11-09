#include "SSAO.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../FrameData.hpp"
#include "../GBufferData.hpp"
#include "../SSAOData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

#include <random>

SSAO::SSAO(RenderContext &rc, uint32_t kernelSize) : m_renderContext{rc} {
  _generateNoiseTexture();
  _generateSampleKernel(kernelSize);
  _createPipeline(kernelSize);
}
SSAO::~SSAO() {
  m_renderContext.destroy(m_pipeline).destroy(m_noise).destroy(m_kernelBuffer);
}

void SSAO::addPass(FrameGraph &fg, FrameGraphBlackboard &blackboard) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;

  blackboard.add<SSAOData>() = fg.addCallbackPass<SSAOData>(
    "SSAO",
    [&](FrameGraph::Builder &builder, SSAOData &data) {
      builder.read(frameBlock);
      builder.read(gBuffer.depth);
      builder.read(gBuffer.normal);

      data.ssao = builder.create<FrameGraphTexture>(
        "SSAO", {.extent = extent, .format = PixelFormat::R8_UNorm});
      data.ssao = builder.write(data.ssao);
    },
    [=](const SSAOData &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("SSAO");
      TracyGpuZone("SSAO");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.ssao),
          .clearValue = glm::vec4{1.0f},
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))

        .bindTexture(0, getTexture(resources, gBuffer.depth))
        .bindTexture(1, getTexture(resources, gBuffer.normal))
        .bindTexture(2, m_noise)

        .bindUniformBuffer(1, m_kernelBuffer)

        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });
}

void SSAO::_generateNoiseTexture() {
  constexpr auto kSize = 4u;

  std::uniform_real_distribution<float> dist{0.0, 1.0};
  std::random_device rd{};
  std::default_random_engine g{rd()};
  std::vector<glm::vec3> ssaoNoise;
  std::generate_n(std::back_inserter(ssaoNoise), kSize * kSize, [&] {
    // Rotate around z-axis (in tangent space)
    return glm::vec3{dist(g) * 2.0 - 1.0, dist(g) * 2.0 - 1.0, 0.0f};
  });

  m_noise =
    m_renderContext.createTexture2D({kSize, kSize}, PixelFormat::RGB16F);
  m_renderContext
    .setupSampler(m_noise,
                  {
                    .minFilter = TexelFilter::Linear,
                    .mipmapMode = MipmapMode::None,
                    .magFilter = TexelFilter::Linear,
                    .addressModeS = SamplerAddressMode::Repeat,
                    .addressModeT = SamplerAddressMode::Repeat,
                  })
    .upload(m_noise, 0, {kSize, kSize},
            {
              .format = GL_RGB,
              .dataType = GL_FLOAT,
              .pixels = ssaoNoise.data(),
            });
}
void SSAO::_generateSampleKernel(uint32_t kernelSize) {
  std::uniform_real_distribution<float> dist{0.0f, 1.0f};
  std::random_device rd{};
  std::default_random_engine g{rd()};

  std::vector<glm::vec4> ssaoKernel;
  std::generate_n(std::back_inserter(ssaoKernel), kernelSize,
                  [&, i = 0]() mutable {
                    glm::vec3 sample{
                      dist(g) * 2.0f - 1.0f,
                      dist(g) * 2.0f - 1.0f,
                      dist(g),
                    };
                    sample = glm::normalize(sample);
                    sample *= dist(g);

                    auto scale = static_cast<float>(i++) / kernelSize;
                    scale = glm::mix(0.1f, 1.0f, scale * scale);
                    return glm::vec4{sample * scale, 0.0f};
                  });

  m_kernelBuffer = m_renderContext.createBuffer(sizeof(glm::vec4) * kernelSize,
                                                ssaoKernel.data());
}
void SSAO::_createPipeline(uint32_t kernelSize) {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.addDefine("KERNEL_SIZE", kernelSize).build("SSAO.frag"));

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
