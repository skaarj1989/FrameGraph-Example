#include "GlobalIllumination.hpp"
#include "Math.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "FrameGraphHelper.hpp"
#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"
#include "GBufferData.hpp"

#include "ShadowCascadesBuilder.hpp"
#include "Grid.hpp"
#include "ShaderCodeBuilder.hpp"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

namespace {

constexpr auto kRSMResolution = 512;
constexpr auto kNumVPL = kRSMResolution * kRSMResolution;

constexpr auto kLPVResolution = 32;

constexpr auto kFirstFreeTextureBinding = 0;

} // namespace

//
// Grid struct:
//

Grid::Grid(const AABB &_aabb) : aabb{_aabb} {
  const auto extent = aabb.getExtent();
  cellSize = max3(extent) / kLPVResolution;
  size = glm::uvec3{extent / cellSize + 0.5f};
}

//
// GlobalIllumination class:
//

GlobalIllumination::GlobalIllumination(RenderContext &rc)
    : BaseGeometryPass{rc} {
  constexpr auto kAdditiveBlending = BlendState{
    .enabled = true,
    .srcColor = BlendFactor::One,
    .destColor = BlendFactor::One,
    .colorOp = BlendOp::Add,

    .srcAlpha = BlendFactor::One,
    .destAlpha = BlendFactor::One,
    .alphaOp = BlendOp::Add,
  };

  // RadianceInjection
  {
    ShaderCodeBuilder shaderCodeBuilder;
    const auto program = m_renderContext.createGraphicsProgram(
      shaderCodeBuilder.build("RadianceInjection.vert"),
      shaderCodeBuilder.build("RadianceInjection.frag"),
      shaderCodeBuilder.build("RadianceInjection.geom"));

    m_radianceInjectionPipeline =
      GraphicsPipeline::Builder{}
        .setDepthStencil({.depthTest = false, .depthWrite = false})
        .setRasterizerState({.cullMode = CullMode::None})
        .setBlendState(0, kAdditiveBlending)
        .setBlendState(1, kAdditiveBlending)
        .setBlendState(2, kAdditiveBlending)
        .setShaderProgram(program)
        .build();
  }

  // RadiancePropagation
  {
    ShaderCodeBuilder shaderCodeBuilder;
    const auto program = m_renderContext.createGraphicsProgram(
      shaderCodeBuilder.build("RadiancePropagation.vert"),
      shaderCodeBuilder.build("RadiancePropagation.frag"),
      shaderCodeBuilder.build("RadiancePropagation.geom"));

    m_radiancePropagationPipeline =
      GraphicsPipeline::Builder{}
        .setDepthStencil({.depthTest = false, .depthWrite = false})
        .setRasterizerState({.cullMode = CullMode::None})
        .setBlendState(0, kAdditiveBlending)
        .setBlendState(1, kAdditiveBlending)
        .setBlendState(2, kAdditiveBlending)
        .setShaderProgram(program)
        .build();
  }

  // DebugPipeline
  {
    ShaderCodeBuilder shaderCodeBuilder;
    const auto program = m_renderContext.createGraphicsProgram(
      shaderCodeBuilder.build("DebugVPL.vert"),
      shaderCodeBuilder.build("DebugVPL.frag"),
      shaderCodeBuilder.build("DebugVPL.geom"));

    m_debugPipeline =
      GraphicsPipeline::Builder{}
        .setDepthStencil({.depthTest = true, .depthWrite = false})
        .setRasterizerState({.cullMode = CullMode::None})
        .setShaderProgram(program)
        .build();
  }
}
GlobalIllumination::~GlobalIllumination() {
  m_renderContext.destroy(m_radianceInjectionPipeline)
    .destroy(m_radiancePropagationPipeline)
    .destroy(m_debugPipeline);
}

void GlobalIllumination::update(
  FrameGraph &fg, FrameGraphBlackboard &blackboard, const Grid &grid,
  const PerspectiveCamera &camera, const Light &light,
  std::span<const Renderable> renderables, uint32_t numPropagations) {

  auto lightView =
    buildCascades(camera, light.direction, 1, 1.0f, kRSMResolution)[0]
      .viewProjMatrix;

  std::vector<const Renderable *> visibleRenderables(renderables.size());
  std::transform(renderables.begin(), renderables.end(),
                 visibleRenderables.begin(),
                 [](const Renderable &r) { return std::addressof(r); });

  const auto RSM = _addReflectiveShadowMapPass(fg, std::move(lightView),
                                               light.color * light.intensity,
                                               std::move(visibleRenderables));
  blackboard.add<ReflectiveShadowMapData>(RSM);

  const auto radiance = _addRadianceInjectionPass(fg, RSM, grid);

  std::optional<LightPropagationVolumesData> LPV;
  for (auto i = 0; i < numPropagations; ++i)
    LPV = _addRadiancePropagationPass(fg, LPV ? *LPV : radiance, grid, i);

  blackboard.add<LightPropagationVolumesData>(*LPV);
}

FrameGraphResource GlobalIllumination::addDebugPass(
  FrameGraph &fg, FrameGraphBlackboard &blackboard, const Grid &grid,
  const PerspectiveCamera &camera, FrameGraphResource target) {
  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto &RSM = blackboard.get<ReflectiveShadowMapData>();
  const auto &LPV = blackboard.get<LightPropagationVolumesData>();

  fg.addCallbackPass(
    "DebugVPL",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(gBuffer.depth);

      builder.read(RSM.position);
      builder.read(RSM.normal);

      builder.read(LPV.r);
      builder.read(LPV.g);
      builder.read(LPV.b);

      target = builder.write(target);
    },
    [=, vp = camera.getViewProjection()](
      const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("DebugVPL");
      TracyGpuZone("DebugVPL");

      const auto extent =
        resources.getDescriptor<FrameGraphTexture>(target).extent;
      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, target),
        }},
        .depthAttachment =
          AttachmentInfo{
            .image = getTexture(resources, gBuffer.depth),
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_debugPipeline)
        .bindTexture(0, getTexture(resources, RSM.position))  // vert
        .bindTexture(1, getTexture(resources, RSM.normal))    // vert

        .bindTexture(2, getTexture(resources, LPV.r)) // frag
        .bindTexture(3, getTexture(resources, LPV.g)) // frag
        .bindTexture(4, getTexture(resources, LPV.b)) // frag

        .setUniform1i("u_RSMResolution", kRSMResolution) // vert
        .setUniformVec3("u_MinCorner", grid.aabb.min)    // vert
        .setUniformVec3("u_GridSize", grid.size)         // vert/frag
        .setUniform1f("u_CellSize", grid.cellSize)       // vert/frag
        .setUniformMat4("u_VP", vp)                      // vert

        .draw(std::nullopt, std::nullopt,
              GeometryInfo{
                .topology = PrimitiveTopology::PointList,
                .numVertices = kNumVPL,
              })
        .endRendering(framebuffer);
    });

  return target;
}

ReflectiveShadowMapData GlobalIllumination::_addReflectiveShadowMapPass(
  FrameGraph &fg, const glm::mat4 &lightViewProjection,
  glm::vec3 lightIntensity, std::vector<const Renderable *> &&renderables) {
  constexpr auto kExtent = Extent2D{kRSMResolution, kRSMResolution};

  const auto data = fg.addCallbackPass<ReflectiveShadowMapData>(
    "ReflectiveShadowMap",
    [&](FrameGraph::Builder &builder, ReflectiveShadowMapData &data) {
      data.depth = builder.create<FrameGraphTexture>(
        "RSM/Depth", {
                       .extent = kExtent,
                       .format = PixelFormat::Depth24,
                     });

      data.position = builder.create<FrameGraphTexture>(
        "RSM/Position", {
                          .extent = kExtent,
                          .format = PixelFormat::RGBA16F,
                          .wrapMode = WrapMode::ClampToOpaqueBlack,
                          .filter = TexelFilter::Nearest,
                        });
      data.normal = builder.create<FrameGraphTexture>(
        "RSM/Normal", {
                        .extent = kExtent,
                        .format = PixelFormat::RGBA16F,
                        .wrapMode = WrapMode::ClampToOpaqueBlack,
                        .filter = TexelFilter::Nearest,
                      });
      data.flux = builder.create<FrameGraphTexture>(
        "RSM/Flux", {
                      .extent = kExtent,
                      .format = PixelFormat::RGBA16F,
                      .wrapMode = WrapMode::ClampToOpaqueBlack,
                      .filter = TexelFilter::Nearest,
                    });

      data.depth = builder.write(data.depth);
      data.position = builder.write(data.position);
      data.normal = builder.write(data.normal);
      data.flux = builder.write(data.flux);
    },
    [=, renderables = std::move(renderables)](
      const ReflectiveShadowMapData &data, FrameGraphPassResources &resources,
      void *ctx) {
      NAMED_DEBUG_MARKER("ReflectiveShadowMap");
      TracyGpuZone("ReflectiveShadowMap");

      constexpr float kFarPlane{1.0f};
      constexpr glm::vec4 kBlackColor{0.0f};

      const RenderingInfo renderingInfo{
        .area = {.extent = kExtent},
        .colorAttachments =
          {
            {
              .image = getTexture(resources, data.position),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.normal),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.flux),
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
      for (const auto *renderable : renderables) {
        const auto &[mesh, subMeshId, material, flags, modelMatrix, _1] =
          *renderable;

        rc.setGraphicsPipeline(_getPipeline(*mesh.vertexFormat, &material));
        _setTransform(lightViewProjection, modelMatrix);

        for (uint32_t unit{kFirstFreeTextureBinding};
             const auto &[_, texture] : material.getDefaultTextures()) {
          rc.bindTexture(unit++, *texture);
        }
        rc.setUniformVec3("u_LightIntensity", lightIntensity) // frag
          .draw(*mesh.vertexBuffer, *mesh.indexBuffer,
                mesh.subMeshes[subMeshId].geometryInfo);
      }
      rc.endRendering(framebuffer);
    });

  return data;
}

LightPropagationVolumesData GlobalIllumination::_addRadianceInjectionPass(
  FrameGraph &fg, const ReflectiveShadowMapData &RSM, const Grid &grid) {
  const auto extent = Extent2D{.width = grid.size.x, .height = grid.size.y};

  const auto data = fg.addCallbackPass<LightPropagationVolumesData>(
    "RadianceInjection",
    [&](FrameGraph::Builder &builder, LightPropagationVolumesData &data) {
      builder.read(RSM.position);
      builder.read(RSM.normal);
      builder.read(RSM.flux);

      data.r = builder.create<FrameGraphTexture>(
        "SH/R", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Nearest,
                });
      data.g = builder.create<FrameGraphTexture>(
        "SH/G", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Nearest,
                });
      data.b = builder.create<FrameGraphTexture>(
        "SH/B", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Nearest,
                });

      data.r = builder.write(data.r);
      data.g = builder.write(data.g);
      data.b = builder.write(data.b);
    },
    [=](const LightPropagationVolumesData &data,
        FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("RadianceInjection");
      TracyGpuZone("RadianceInjection");

      constexpr glm::vec4 kBlackColor{0.0f};

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments =
          {
            {
              .image = getTexture(resources, data.r),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.g),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.b),
              .clearValue = kBlackColor,
            },
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);

      rc.setGraphicsPipeline(m_radianceInjectionPipeline)
        .bindTexture(0, getTexture(resources, RSM.position))
        .bindTexture(1, getTexture(resources, RSM.normal))
        .bindTexture(2, getTexture(resources, RSM.flux))

        .setUniform1i("u_RSMResolution", kRSMResolution) // vert
        .setUniformVec3("u_MinCorner", grid.aabb.min)    // vert
        .setUniformVec3("u_GridSize", grid.size)         // vert
        .setUniform1f("u_CellSize", grid.cellSize)       // vert/frag

        .draw(std::nullopt, std::nullopt,
              GeometryInfo{
                .topology = PrimitiveTopology::PointList,
                .numVertices = kNumVPL,
              });

      rc.endRendering(framebuffer);
    });

  return data;
}

LightPropagationVolumesData GlobalIllumination::_addRadiancePropagationPass(
  FrameGraph &fg, const LightPropagationVolumesData &LPV, const Grid &grid,
  uint32_t iteration) {
  const auto name = "RadiancePropagation #" + std::to_string(iteration);
  const auto extent = Extent2D{.width = grid.size.x, .height = grid.size.y};

  const auto data = fg.addCallbackPass<LightPropagationVolumesData>(
    name,
    [&](FrameGraph::Builder &builder, LightPropagationVolumesData &data) {
      builder.read(LPV.r);
      builder.read(LPV.g);
      builder.read(LPV.b);

      data.r = builder.create<FrameGraphTexture>(
        "SH/R", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Linear,
                });
      data.g = builder.create<FrameGraphTexture>(
        "SH/G", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Linear,
                });
      data.b = builder.create<FrameGraphTexture>(
        "SH/B", {
                  .extent = extent,
                  .depth = grid.size.z,
                  .format = PixelFormat::RGBA16F,
                  .wrapMode = WrapMode::ClampToOpaqueBlack,
                  .filter = TexelFilter::Linear,
                });

      data.r = builder.write(data.r);
      data.g = builder.write(data.g);
      data.b = builder.write(data.b);
    },
    [=](const LightPropagationVolumesData &data,
        FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("RadiancePropagation");
      TracyGpuZone("RadiancePropagation");

      constexpr glm::vec4 kBlackColor{0.0f};

      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments =
          {
            {
              .image = getTexture(resources, data.r),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.g),
              .clearValue = kBlackColor,
            },
            {
              .image = getTexture(resources, data.b),
              .clearValue = kBlackColor,
            },
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);

      rc.setGraphicsPipeline(m_radiancePropagationPipeline);

      rc.bindTexture(0, getTexture(resources, LPV.r)) // frag
        .bindTexture(1, getTexture(resources, LPV.g)) // frag
        .bindTexture(2, getTexture(resources, LPV.b)) // frag

        .setUniformVec3("u_GridSize", grid.size) // vert/frag

        .draw(std::nullopt, std::nullopt,
              GeometryInfo{
                .topology = PrimitiveTopology::PointList,
                .numVertices = kNumVPL,
              })

        .endRendering(framebuffer);
    });

  return data;
}

GraphicsPipeline
GlobalIllumination::_createBasePassPipeline(const VertexFormat &vertexFormat,
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
      .addDefine("BLEND_MODE", static_cast<int32_t>(material->getBlendMode()))
      .replace("#pragma USER_SAMPLERS",
               getSamplersChunk(material->getDefaultTextures(),
                                kFirstFreeTextureBinding))
      .replace("#pragma USER_CODE", material->getUserFragCode())
      .build("ReflectiveShadowMapPass.frag");

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
      .cullMode = CullMode::Back,
      .depthClampEnable = false,
      .scissorTest = false,
    })
    .setVertexArray(vao)
    .setShaderProgram(program)
    .build();
}
