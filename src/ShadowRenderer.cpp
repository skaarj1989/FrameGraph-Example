#include "ShadowRenderer.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "FrameGraphHelper.hpp"
#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"

#include "FrameData.hpp"
#include "GBufferData.hpp"
#include "ShadowMapData.hpp"

#include "ShadowCascadesBuilder.hpp"
#include "ShaderCodeBuilder.hpp"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

namespace {

constexpr auto kFirstFreeTextureBinding = 0;

constexpr auto kShadowMapSize = 1024;
constexpr auto kNumCascades = 4;

static_assert(kNumCascades <= 4);
struct GPUCascades {
  glm::vec4 splitDepth;
  glm::mat4 viewProjMatrices[kNumCascades];
};

// clang-format off
#if GLM_CONFIG_CLIP_CONTROL & GLM_CLIP_CONTROL_ZO_BIT
const glm::mat4 kBiasMatrix{
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.5, 0.5, 0.0, 1.0
};
#else
const glm::mat4 kBiasMatrix{
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 0.5, 0.0,
  0.5, 0.5, 0.5, 1.0
};
#endif
// clang-format on

template <typename Func>
requires std::is_invocable_v<Func, const AABB &>
[[nodiscard]] auto
getVisibleShadowCasters(std::span<const Renderable> renderables,
                        const glm::mat4 &viewProj, Func isVisible) {
  ZoneScoped;

  std::vector<const Renderable *> result;
  for (const auto &renderable : renderables) {
    if ((renderable.flags & MaterialFlag_CastShadow) && isOpaque(&renderable) &&
        isVisible(renderable.aabb)) {
      result.push_back(&renderable);
    }
  }
  return result;
}

void uploadCascades(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                    std::vector<Cascade> &&cascades) {
  auto &shadowMatrices = blackboard.get<ShadowMapData>().viewProjMatrices;

  fg.addCallbackPass(
    "UploadCascades",
    [&](FrameGraph::Builder &builder, auto &) {
      shadowMatrices = builder.write(shadowMatrices);
    },
    [=, cascades = std::move(cascades)](
      const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("UploadCascades");
      TracyGpuZone("UploadCascades");

      GPUCascades gpuCascades{};
      for (uint32_t i{0}; i < cascades.size(); ++i) {
        gpuCascades.splitDepth[i] = cascades[i].splitDepth;
        gpuCascades.viewProjMatrices[i] =
          kBiasMatrix * cascades[i].viewProjMatrix;
      }
      static_cast<RenderContext *>(ctx)->upload(
        getBuffer(resources, shadowMatrices), 0, sizeof(GPUCascades),
        &gpuCascades);
    });
}

} // namespace

//
// ShadowRenderer class:
//

ShadowRenderer::ShadowRenderer(RenderContext &rc) : BaseGeometryPass{rc} {
  m_shadowMatrices = rc.createBuffer(sizeof(GPUCascades));

  const uint16_t kPixels[1 * 1]{UINT16_MAX};
  m_dummyShadowMaps = rc.createTexture2D({1, 1}, PixelFormat::Depth16, 1, 1);
  rc.upload(m_dummyShadowMaps, 0, {1, 1, 0}, 0, 0,
            {
              .format = GL_DEPTH_COMPONENT,
              .dataType = GL_UNSIGNED_SHORT,
              .pixels = kPixels,
            })
    .setupSampler(m_dummyShadowMaps,
                  {
                    .minFilter = TexelFilter::Linear,
                    .mipmapMode = MipmapMode::None,
                    .magFilter = TexelFilter::Linear,
                    .addressModeS = SamplerAddressMode::ClampToBorder,
                    .addressModeT = SamplerAddressMode::ClampToBorder,
                    .compareOp = CompareOp::LessOrEqual,
                    .borderColor = glm::vec4{1.0f},
                  });

  _setupDebugPipeline();
}
ShadowRenderer::~ShadowRenderer() {
  m_renderContext.destroy(m_debugPipeline)
    .destroy(m_dummyShadowMaps)
    .destroy(m_shadowMatrices);
}

void ShadowRenderer::buildCascadedShadowMaps(
  FrameGraph &fg, FrameGraphBlackboard &blackboard,
  const PerspectiveCamera &camera, const Light *light,
  std::span<const Renderable> renderables) {
  auto &shadowMapData = blackboard.add<ShadowMapData>();
  shadowMapData.viewProjMatrices =
    importBuffer(fg, "CascadeMatrices", &m_shadowMatrices);

  if (light == nullptr) {
    shadowMapData.cascadedShadowMaps =
      importTexture(fg, "DummyShadowMaps", &m_dummyShadowMaps);
  } else {
    auto cascades = buildCascades(camera, light->direction, kNumCascades, 0.94f,
                                  kShadowMapSize);

    std::optional<FrameGraphResource> cascadedShadowMaps;
    for (uint32_t i{0}; i < cascades.size(); ++i) {
      const auto &lightViewProj = cascades[i].viewProjMatrix;
      auto visibleShadowCasters = getVisibleShadowCasters(
        renderables, lightViewProj,
        [frustum = Frustum{lightViewProj}](const AABB &aabb) {
          return frustum.testAABB(aabb);
        });
      // The first iteration will be responsible for creation of the shadowmap
      // (Texture2DArray)
      cascadedShadowMaps =
        _addCascadePass(fg, cascadedShadowMaps, lightViewProj,
                        std::move(visibleShadowCasters), i);
    }
    assert(cascadedShadowMaps);
    shadowMapData.cascadedShadowMaps = *cascadedShadowMaps;
    uploadCascades(fg, blackboard, std::move(cascades));
  }
}

FrameGraphResource ShadowRenderer::visualizeCascades(
  FrameGraph &fg, FrameGraphBlackboard &blackboard, FrameGraphResource target) {
  const auto [frameBlock] = blackboard.get<FrameData>();
  const auto depth = blackboard.get<GBufferData>().depth;
  const auto cascades = blackboard.get<ShadowMapData>().viewProjMatrices;

  fg.addCallbackPass(
    "VisualizeCascades",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(frameBlock);
      builder.read(depth);
      builder.read(cascades);

      target = builder.write(target);
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("VisualizeCascades");
      TracyGpuZone("VisualizeCascades");

      const auto extent =
        resources.getDescriptor<FrameGraphTexture>(target).extent;
      const RenderingInfo renderingInfo{
        .area = {.extent = extent},
        .colorAttachments = {{
          .image = getTexture(resources, target),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_debugPipeline)
        .bindUniformBuffer(0, getBuffer(resources, frameBlock))
        .bindTexture(0, getTexture(resources, depth))
        .bindUniformBuffer(1, getBuffer(resources, cascades))
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return target;
}

void ShadowRenderer::_setupDebugPipeline() {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("VisualizeCascadeSplits.frag"));

  m_debugPipeline =
    GraphicsPipeline::Builder{}
      .setDepthStencil({.depthTest = false, .depthWrite = false})
      .setRasterizerState({.cullMode = CullMode::Back})
      .setBlendState(0,
                     {
                       .enabled = true,
                       .srcColor = BlendFactor::SrcAlpha,
                       .destColor = BlendFactor::OneMinusSrcAlpha,
                       .colorOp = BlendOp::Add,

                       .srcAlpha = BlendFactor::One,
                       .destAlpha = BlendFactor::One,
                       .alphaOp = BlendOp::Add,
                     })
      .setShaderProgram(program)
      .build();
}

GraphicsPipeline
ShadowRenderer::_createBasePassPipeline(const VertexFormat &vertexFormat,
                                        const Material *material) {
  assert(material);

  const auto vao = m_renderContext.getVertexArray(vertexFormat.getAttributes());

  ShaderCodeBuilder shaderCodeBuilder;
  shaderCodeBuilder.setDefines(buildDefines(vertexFormat))
    .addDefine("DEPTH_PASS", 1);

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
      .build("DepthPass.frag");

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
      .cullMode = CullMode::Front,
      .depthClampEnable = true,
      .scissorTest = false,
    })
    .setVertexArray(vao)
    .setShaderProgram(program)
    .build();
}

FrameGraphResource ShadowRenderer::_addCascadePass(
  FrameGraph &fg, std::optional<FrameGraphResource> cascadedShadowMaps,
  const glm::mat4 &lightViewProj, std::vector<const Renderable *> &&renderables,
  uint32_t cascadeIdx) {
  assert(cascadeIdx < kNumCascades);
  const auto name = "CSM #" + std::to_string(cascadeIdx);

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    name,
    [&](FrameGraph::Builder &builder, Data &data) {
      if (cascadeIdx == 0) {
        assert(!cascadedShadowMaps);
        cascadedShadowMaps = builder.create<FrameGraphTexture>(
          "CascadedShadowMaps", {
                                  .extent = {kShadowMapSize, kShadowMapSize},
                                  .layers = kNumCascades,
                                  .format = PixelFormat::Depth24,
                                  .shadowSampler = true,
                                });
      }
      data.output = builder.write(*cascadedShadowMaps);
    },
    [=, renderables = std::move(renderables)](
      const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER(name);
      TracyGpuZone("CSM");

      constexpr float kFarPlane{1.0f};
      const RenderingInfo renderingInfo{
        .area = {.extent = {kShadowMapSize, kShadowMapSize}},
        .depthAttachment =
          AttachmentInfo{
            .image = getTexture(resources, data.output),
            .layer = cascadeIdx,
            .clearValue = kFarPlane,
          },
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      for (const auto *renderable : renderables) {
        const auto &[mesh, subMeshIndex, material, _0, modelMatrix, _1] =
          *renderable;

        rc.setGraphicsPipeline(_getPipeline(*mesh.vertexFormat, &material));
        _setTransform(lightViewProj, modelMatrix);
        rc.draw(*mesh.vertexBuffer, *mesh.indexBuffer,
                mesh.subMeshes[subMeshIndex].geometryInfo);
      }
      rc.endRendering(framebuffer);
    });

  return pass.output;
}
