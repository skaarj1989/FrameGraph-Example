#include "GBufferPass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../FrameData.hpp"
#include "../GBufferData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

namespace {

constexpr auto kFirstFreeTextureBinding = 0;

}

GBufferPass::GBufferPass(RenderContext &rc) : BaseGeometryPass{rc} {}

void GBufferPass::addGeometryPass(FrameGraph &fg,
                                  FrameGraphBlackboard &blackboard,
                                  Extent2D resolution,
                                  const PerspectiveCamera &camera,
                                  std::span<const Renderable *> renderables) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  blackboard.add<GBufferData>() = fg.addCallbackPass<GBufferData>(
    "GBuffer Pass",
    [&](FrameGraph::Builder &builder, GBufferData &data) {
      builder.read(frameBlock);

      data.depth = builder.create<FrameGraphTexture>(
        "SceneDepth", {.extent = resolution, .format = PixelFormat::Depth24});
      data.depth = builder.write(data.depth);

      data.normal = builder.create<FrameGraphTexture>(
        "Normal", {.extent = resolution, .format = PixelFormat::RGB16F});
      data.normal = builder.write(data.normal);

      data.albedo = builder.create<FrameGraphTexture>(
        "Albedo SpecularWeight",
        {.extent = resolution, .format = PixelFormat::RGBA8_UNorm});
      data.albedo = builder.write(data.albedo);

      data.emissive = builder.create<FrameGraphTexture>(
        "Emissive", {.extent = resolution, .format = PixelFormat::RGB16F});
      data.emissive = builder.write(data.emissive);

      data.metallicRoughnessAO = builder.create<FrameGraphTexture>(
        "Metallic Roughness AO",
        {.extent = resolution, .format = PixelFormat::RGBA8_UNorm});
      data.metallicRoughnessAO = builder.write(data.metallicRoughnessAO);
    },
    [=, this, camera = &camera](const GBufferData &data,
                                FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("GBuffer");
      TracyGpuZone("GBuffer");

      constexpr glm::vec4 kBlackColor{0.0f};
      constexpr float kFarPlane{1.0f};
      const RenderingInfo renderingInfo{
        .area = {.extent = resolution},
        .colorAttachments =
          {
            {
              .image = getTexture(resources, data.normal),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.albedo),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.emissive),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.metallicRoughnessAO),
              .clearValue = kBlackColor,
            },
          },
        .depthAttachment =
          AttachmentInfo{
            .image = getTexture(resources, data.depth),
            .clearValue = kFarPlane,
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      for (const auto &renderable : renderables) {
        auto &[mesh, subMeshIndex, material, flags, modelMatrix, _] =
          *renderable;

        rc.setGraphicsPipeline(_getPipeline(*mesh.vertexFormat, &material))
          .bindUniformBuffer(0, getBuffer(resources, frameBlock));

        _setTransform(*camera, modelMatrix);
        for (uint32_t unit{kFirstFreeTextureBinding};
             const auto &[_, texture] : material.getDefaultTextures()) {
          rc.bindTexture(unit++, *texture);
        }
        rc.setUniform1i("u_MaterialFlags", flags)
          .draw(*mesh.vertexBuffer, *mesh.indexBuffer,
                mesh.subMeshes[subMeshIndex].geometryInfo);
      }
      rc.endRendering(framebuffer);
    });
}

GraphicsPipeline
GBufferPass::_createBasePassPipeline(const VertexFormat &vertexFormat,
                                     const Material *material) {
  assert(material);

  const auto vao = m_renderContext.getVertexArray(vertexFormat.getAttributes());

  ShaderCodeBuilder shaderCodeBuilder;
  shaderCodeBuilder.setDefines(buildDefines(vertexFormat));

  const auto vertCode =
    shaderCodeBuilder.replace("#pragma USER_CODE", material->getUserVertCode())
      .build("Geometry.vert");

  const auto fragCode =
    shaderCodeBuilder
      .addDefine("SHADING_MODEL",
                 static_cast<uint32_t>(material->getShadingModel()))
      .addDefine("BLEND_MODE", static_cast<int32_t>(material->getBlendMode()))
      .replace("#pragma USER_SAMPLERS",
               getSamplersChunk(material->getDefaultTextures(),
                                kFirstFreeTextureBinding))
      .replace("#pragma USER_CODE", material->getUserFragCode())
      .build("GBufferPass.frag");

  const auto program =
    m_renderContext.createGraphicsProgram(vertCode, fragCode);

  return GraphicsPipeline::Builder{}
    .setDepthStencil({
      .depthTest = true,
      .depthWrite = true,
      .depthCompareOp = CompareOp::LessOrEqual,
    })
    .setRasterizerState({
      .polygonMode = PolygonMode::Fill,
      .cullMode = material->getCullMode(),
      .scissorTest = false,
    })
    .setVertexArray(vao)
    .setShaderProgram(program)
    .build();
}
