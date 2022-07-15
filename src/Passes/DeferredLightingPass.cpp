#include "DeferredLightingPass.hpp"

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
#include "../LightPropagationVolumesData.hpp"
#include "../GBufferData.hpp"
#include "../SSAOData.hpp"

#include "../Grid.hpp"
#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

DeferredLightingPass::DeferredLightingPass(RenderContext &rc, uint32_t tileSize)
    : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.addDefine("TILED_LIGHTING", 1)
                               .addDefine("TILE_SIZE", tileSize)
                               .build("DeferredLighting.frag"));

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
DeferredLightingPass::~DeferredLightingPass() {
  m_renderContext.destroy(m_pipeline);
}

FrameGraphResource
DeferredLightingPass::addPass(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                              const Grid &grid) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;

  const auto &brdf = blackboard.get<BRDF>();
  const auto &globalLightProbe = blackboard.get<GlobalLightProbeData>();

  const auto &lights = blackboard.get<LightsData>();
  const auto &lightCulling = blackboard.get<LightCullingData>();

  const auto maybeSSAO = blackboard.try_get<SSAOData>();

  const auto &cascades = blackboard.get<ShadowMapData>();
  const auto *LPV = blackboard.try_get<LightPropagationVolumesData>();

  struct Data {
    FrameGraphResource sceneColor;
  };
  auto &deferredLighting = fg.addCallbackPass<Data>(
    "DeferredLighting Pass",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(frameBlock);

      builder.read(gBuffer.depth);
      builder.read(gBuffer.normal);
      builder.read(gBuffer.albedo);
      builder.read(gBuffer.emissive);
      builder.read(gBuffer.metallicRoughnessAO);

      builder.read(brdf.lut);
      builder.read(globalLightProbe.diffuse);
      builder.read(globalLightProbe.specular);

      builder.read(lights.buffer);
      builder.read(lightCulling.lightGrid);
      builder.read(lightCulling.lightIndices);

      builder.read(cascades.cascadedShadowMaps);
      builder.read(cascades.viewProjMatrices);

      if (LPV) {
        builder.read(LPV->r);
        builder.read(LPV->g);
        builder.read(LPV->b);
      }

      if (maybeSSAO) builder.read(maybeSSAO->ssao);

      data.sceneColor = builder.create<FrameGraphTexture>(
        "SceneColorHDR", {.extent = extent, .format = PixelFormat::RGB16F});
      data.sceneColor = builder.write(data.sceneColor);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("DeferredLighting");
      TracyGpuZone("DeferredLighting");

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.sceneColor),
          .clearValue = glm::vec4{0.0f},
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))

        .bindTexture(0, getTexture(resources, gBuffer.depth))
        .bindTexture(1, getTexture(resources, gBuffer.normal))
        .bindTexture(2, getTexture(resources, gBuffer.albedo))
        .bindTexture(3, getTexture(resources, gBuffer.emissive))
        .bindTexture(4, getTexture(resources, gBuffer.metallicRoughnessAO))

        .bindTexture(6, getTexture(resources, brdf.lut))
        .bindTexture(7, getTexture(resources, globalLightProbe.diffuse))
        .bindTexture(8, getTexture(resources, globalLightProbe.specular))

        .bindStorageBuffer(0, getBuffer(resources, lights.buffer))
        .bindStorageBuffer(1, getBuffer(resources, lightCulling.lightIndices))

        .bindImage(0, getTexture(resources, lightCulling.lightGrid), 0,
                   GL_READ_ONLY);

      if (maybeSSAO) {
        rc.bindTexture(5, getTexture(resources, maybeSSAO->ssao));
      }

      rc.bindTexture(9, getTexture(resources, cascades.cascadedShadowMaps))
        .bindUniformBuffer(1, getBuffer(resources, cascades.viewProjMatrices));

      if (LPV) {
        rc.bindTexture(10, getTexture(resources, LPV->r))
          .bindTexture(11, getTexture(resources, LPV->g))
          .bindTexture(12, getTexture(resources, LPV->b));

        rc.setUniformVec3("u_MinCorner", grid.aabb.min)
          .setUniformVec3("u_GridSize", grid.size)
          .setUniform1f("u_CellSize", grid.cellSize);
      }

      rc.drawFullScreenTriangle().endRendering(framebuffer);
    });

  return deferredLighting.sceneColor;
}
