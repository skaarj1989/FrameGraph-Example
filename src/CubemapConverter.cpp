#include "CubemapConverter.hpp"
#include "Math.hpp"
#include "CubeCapture.hpp"
#include "ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

namespace {

[[nodiscard]] static uint32_t
calculateCubeSize(const Texture &equirectangular) {
  assert(equirectangular.getType() == GL_TEXTURE_2D);
  const auto x =
    8 *
    (glm::floor(2 * equirectangular.getExtent().height / glm::pi<float>() / 8));
  // or: width / glm::pi<float>();
  // or: (glm::sqrt(3) / 6) * width;

  return nextPowerOf2(x);
}

} // namespace

//
// CubemapConverter class:
//

CubemapConverter::CubemapConverter(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("Cube.vert"),
    shaderCodeBuilder.build("EquirectangularToCubemap.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setDepthStencil({.depthTest = false, .depthWrite = false})
                 .setRasterizerState({.cullMode = CullMode::Front})
                 .setShaderProgram(program)
                 .build();
}
CubemapConverter::~CubemapConverter() { m_renderContext.destroy(m_pipeline); }

Texture
CubemapConverter::equirectangularToCubemap(const Texture &equirectangular) {
  TracyGpuZone("Equirectangular -> Cubemap");

  const auto size = calculateCubeSize(equirectangular);
  auto cubemap = m_renderContext.createCubemap(size, PixelFormat::RGB16F, 0);
  m_renderContext.setupSampler(
    cubemap, {
               .minFilter = TexelFilter::Linear,
               .mipmapMode = MipmapMode::Linear,
               .magFilter = TexelFilter::Linear,
               .addressModeS = SamplerAddressMode::ClampToEdge,
               .addressModeT = SamplerAddressMode::ClampToEdge,
               .addressModeR = SamplerAddressMode::ClampToEdge,
             });

  for (uint8_t face{0}; face < 6; ++face) {
    const RenderingInfo renderingInfo{
      .area = {.extent = {size, size}},
      .colorAttachments = {AttachmentInfo{
        .image = cubemap,
        .face = face,
        .clearValue = glm::vec4{0.0f},
      }},
    };
    const auto framebuffer = m_renderContext.beginRendering(renderingInfo);
    m_renderContext.setGraphicsPipeline(m_pipeline)
      .bindTexture(0, equirectangular)
      .setUniformMat4("u_ViewProjMatrix", kCubeProjection * kCaptureViews[face])
      .drawCube()
      .endRendering(framebuffer);
  }
  m_renderContext.generateMipmaps(cubemap);

  return cubemap;
}
