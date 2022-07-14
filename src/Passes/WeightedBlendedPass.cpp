#include "WeightedBlendedPass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"
#include "../FrameGraphBuffer.hpp"

#include "../FrameData.hpp"
#include "../BRDF.hpp"
#include "../GlobalLightProbeData.hpp"
#include "../LightsData.hpp"
#include "../LightCullingData.hpp"
#include "../ShadowMapData.hpp"
#include "../GBufferData.hpp"
#include "../WeightedBlendedData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

namespace {

constexpr auto kFirstFreeTextureBinding = 5;

}

WeightedBlendedPass::WeightedBlendedPass(RenderContext &rc, uint32_t tileSize)
    : BaseGeometryPass{rc}, m_tileSize{tileSize} {}

void WeightedBlendedPass::addPass(FrameGraph &fg,
                                  FrameGraphBlackboard &blackboard,
                                  const PerspectiveCamera &camera,
                                  std::span<const Renderable *> renderables) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;

  const auto &brdf = blackboard.get<BRDF>();
  const auto &globalLightProbe = blackboard.get<GlobalLightProbeData>();

  const auto &lights = blackboard.get<LightsData>();
  const auto &lightCulling = blackboard.get<LightCullingData>();

  const auto &cascades = blackboard.get<ShadowMapData>();

  blackboard.add<WeightedBlendedData>() =
    fg.addCallbackPass<WeightedBlendedData>(
      "WeightedBlended OIT",
      [&](FrameGraph::Builder &builder, WeightedBlendedData &data) {
        builder.read(frameBlock);

        builder.read(gBuffer.depth);

        builder.read(brdf.lut);
        builder.read(globalLightProbe.diffuse);
        builder.read(globalLightProbe.specular);

        builder.read(lights.buffer);
        builder.read(lightCulling.lightGrid);
        builder.read(lightCulling.lightIndices);

        builder.read(cascades.cascadedShadowMaps);
        builder.read(cascades.viewProjMatrices);

        data.accum = builder.create<FrameGraphTexture>(
          "Accum", {.extent = extent, .format = PixelFormat::RGBA16F});
        data.accum = builder.write(data.accum);

        data.reveal = builder.create<FrameGraphTexture>(
          "Reveal", {.extent = extent, .format = PixelFormat::R8_UNorm});
        data.reveal = builder.write(data.reveal);
      },
      [=, camera = &camera](const WeightedBlendedData &data,
                            FrameGraphPassResources &resources, void *ctx) {
        NAMED_DEBUG_MARKER("WeightedBlended OIT");
        TracyGpuZone("WeightedBlended OIT");

        const RenderingInfo renderingInfo{
          .area = {.extent = extent},
          .colorAttachments =
            {
              {
                .image = getTexture(resources, data.accum),
                .clearValue = glm::vec4{0.0f},
              },
              {
                .image = getTexture(resources, data.reveal),
                .clearValue = glm::vec4{1.0f},
              },
            },
          .depthAttachment =
            AttachmentInfo{
              .image = getTexture(resources, gBuffer.depth),
            },
        };
        auto &rc = *static_cast<RenderContext *>(ctx);
        const auto framebuffer = rc.beginRendering(renderingInfo);
        rc.bindUniformBuffer(0, getBuffer(resources, frameBlock))

          .bindTexture(0, getTexture(resources, gBuffer.depth))

          .bindTexture(1, getTexture(resources, brdf.lut))
          .bindTexture(2, getTexture(resources, globalLightProbe.diffuse))
          .bindTexture(3, getTexture(resources, globalLightProbe.specular))

          .bindStorageBuffer(0, getBuffer(resources, lights.buffer))
          .bindStorageBuffer(1, getBuffer(resources, lightCulling.lightIndices))

          .bindImage(0, getTexture(resources, lightCulling.lightGrid), 0,
                     GL_READ_ONLY)

          .bindTexture(4, getTexture(resources, cascades.cascadedShadowMaps))
          .bindUniformBuffer(1,
                             getBuffer(resources, cascades.viewProjMatrices));

        for (const auto *renderable : renderables) {
          const auto &[mesh, subMeshIndex, material, flags, modelMatrix, _] =
            *renderable;

          rc.setGraphicsPipeline(_getPipeline(*mesh.vertexFormat, &material));
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
WeightedBlendedPass::_createBasePassPipeline(const VertexFormat &vertexFormat,
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
      .addDefine("TILED_LIGHTING", 1)
      .addDefine("TILE_SIZE", m_tileSize)
      .replace("#pragma USER_SAMPLERS",
               getSamplersChunk(material->getDefaultTextures(),
                                kFirstFreeTextureBinding))
      .replace("#pragma USER_CODE", material->getUserFragCode())
      .build("WeightedBlendedPass.frag");

  const auto program =
    m_renderContext.createGraphicsProgram(vertCode, fragCode);

  return GraphicsPipeline::Builder{}
    .setDepthStencil({
      .depthTest = true,
      .depthWrite = false,
      .depthCompareOp = CompareOp::LessOrEqual,
    })
    .setBlendState(0,
                   {
                     .enabled = true,
                     .srcColor = BlendFactor::One,
                     .destColor = BlendFactor::One,
                     .srcAlpha = BlendFactor::One,
                     .destAlpha = BlendFactor::One,
                   })
    .setBlendState(1,
                   {
                     .enabled = true,
                     .srcColor = BlendFactor::Zero,
                     .destColor = BlendFactor::OneMinusSrcColor,
                     .srcAlpha = BlendFactor::Zero,
                     .destAlpha = BlendFactor::OneMinusSrcColor,
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
