#include "IBL.hpp"
#include "CubeCapture.hpp"
#include "ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

//
// IBL class:
//

IBL::IBL(RenderContext &rc) : m_renderContext{rc} {
  GraphicsPipeline::Builder builder{};
  builder.setDepthStencil({.depthTest = false, .depthWrite = false});

  ShaderCodeBuilder shaderCodeBuilder;
  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("IBL/GenerateBRDF.frag"));
  m_brdfPipeline = builder.setShaderProgram(program).build();

  const auto cubeCodeVS = shaderCodeBuilder.build("Cube.vert");
  program = m_renderContext.createGraphicsProgram(
    cubeCodeVS, shaderCodeBuilder.build("IBL/GenerateIrradianceMap.frag"));
  m_irradiancePipeline =
    builder.setRasterizerState({.cullMode = CullMode::Front})
      .setShaderProgram(program)
      .build();

  program = m_renderContext.createGraphicsProgram(
    cubeCodeVS, shaderCodeBuilder.build("IBL/PrefilterEnvMap.frag"));
  m_prefilterEnvMapPipeline = builder.setShaderProgram(program).build();
}
IBL::~IBL() {
  m_renderContext.destroy(m_brdfPipeline)
    .destroy(m_irradiancePipeline)
    .destroy(m_prefilterEnvMapPipeline);
}

Texture IBL::generateBRDF() {
  TracyGpuZone("GenerateBRDF");

  constexpr auto kSize = 512u;
  auto brdf =
    m_renderContext.createTexture2D({kSize, kSize}, PixelFormat::RG16F);
  m_renderContext.setupSampler(
    brdf, {
            .minFilter = TexelFilter::Linear,
            .mipmapMode = MipmapMode::None,
            .magFilter = TexelFilter::Linear,
            .addressModeS = SamplerAddressMode::ClampToEdge,
            .addressModeT = SamplerAddressMode::ClampToEdge,
          });

  const RenderingInfo renderingInfo{
    .area = {.extent = {kSize, kSize}},
    .colorAttachments = {{.image = brdf}},
  };
  const auto framebuffer = m_renderContext.beginRendering(renderingInfo);
  m_renderContext.setGraphicsPipeline(m_brdfPipeline)
    .drawFullScreenTriangle()
    .endRendering(framebuffer);

  return brdf;
}

Texture IBL::generateIrradiance(const Texture &cubemap) {
  assert(cubemap.getType() == GL_TEXTURE_CUBE_MAP);

  TracyGpuZone("GenerateIrradiance");

  constexpr auto kSize = 64u;
  auto irradiance =
    m_renderContext.createCubemap(kSize, PixelFormat::RGB16F, 0);
  m_renderContext.setupSampler(
    irradiance, {
                  .minFilter = TexelFilter::Linear,
                  .mipmapMode = MipmapMode::None,
                  .magFilter = TexelFilter::Linear,
                  .addressModeS = SamplerAddressMode::ClampToEdge,
                  .addressModeT = SamplerAddressMode::ClampToEdge,
                  .addressModeR = SamplerAddressMode::ClampToEdge,
                });

  for (uint8_t face{0}; face < 6; ++face) {
    const RenderingInfo renderingInfo{
      .area = {.extent = {kSize, kSize}},
      .colorAttachments = {AttachmentInfo{
        .image = irradiance,
        .face = face,
      }},
    };
    const auto framebuffer = m_renderContext.beginRendering(renderingInfo);
    m_renderContext.setGraphicsPipeline(m_irradiancePipeline)
      .bindTexture(0, cubemap)
      .setUniformMat4("u_ViewProjMatrix", kCubeProjection * kCaptureViews[face])
      .drawCube()
      .endRendering(framebuffer);
  }
  m_renderContext.generateMipmaps(irradiance);

  return irradiance;
}
Texture IBL::prefilterEnvMap(const Texture &cubemap) {
  assert(cubemap.getType() == GL_TEXTURE_CUBE_MAP);

  TracyGpuZone("PrefilterEnvMap");

  constexpr auto kSize = 512u;
  auto prefilteredEnvMap =
    m_renderContext.createCubemap(kSize, PixelFormat::RGB16F, 0);
  m_renderContext.setupSampler(
    prefilteredEnvMap, {
                         .minFilter = TexelFilter::Linear,
                         .mipmapMode = MipmapMode::Linear,
                         .magFilter = TexelFilter::Linear,
                         .addressModeS = SamplerAddressMode::ClampToEdge,
                         .addressModeT = SamplerAddressMode::ClampToEdge,
                         .addressModeR = SamplerAddressMode::ClampToEdge,
                       });

  for (uint8_t level{0}; level < prefilteredEnvMap.getNumMipLevels(); ++level) {
    const auto mipSize = calcMipSize(glm::uvec3{kSize, kSize, 1u}, level).x;
    for (uint8_t face{0}; face < 6; ++face) {
      const RenderingInfo renderingInfo{
        .area = {.extent = {mipSize, mipSize}},
        .colorAttachments = {AttachmentInfo{
          .image = prefilteredEnvMap,
          .mipLevel = level,
          .face = face,
        }},
      };
      const auto framebuffer = m_renderContext.beginRendering(renderingInfo);
      m_renderContext.setGraphicsPipeline(m_prefilterEnvMapPipeline)
        .bindTexture(0, cubemap)
        .setUniform1f("u_Roughness",
                      static_cast<float>(level) /
                        (prefilteredEnvMap.getNumMipLevels() - 1u))
        .setUniformMat4("u_ViewProjMatrix",
                        kCubeProjection * kCaptureViews[face])
        .drawCube()
        .endRendering(framebuffer);
    }
  }

  return prefilteredEnvMap;
}
