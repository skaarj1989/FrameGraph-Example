#include "GlobalIllumination.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "FrameGraphHelper.hpp"
#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"
#include "imgui.h"
#include "GBufferData.hpp"

#include "ShadowCascadesBuilder.hpp"

#include "ShaderCodeBuilder.hpp"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

namespace {

constexpr auto kRSMResolution = 512;
constexpr auto kNumVPL = kRSMResolution * kRSMResolution;

constexpr auto kLPVResolution = 64;

constexpr auto kFirstFreeTextureBinding = 0;

} // namespace

GlobalIllumination::GlobalIllumination(RenderContext &rc)
    : BaseGeometryPass{rc} {}
GlobalIllumination::~GlobalIllumination() {}

void GlobalIllumination::update(FrameGraph &fg, const PerspectiveCamera &camera,
                                const Light &light,
                                std::span<const Renderable> renderables) {
  constexpr auto kNumCascades = 4;
  constexpr int32_t kCascadeIdx = kNumCascades * 0.5f;

  auto lightView = buildCascades(camera, light.direction, kNumCascades, 0.94,
                                 kRSMResolution)[kCascadeIdx]
                     .viewProjMatrix;

  std::vector<const Renderable *> visibleRenderables(renderables.size());
  std::transform(renderables.begin(), renderables.end(),
                 visibleRenderables.begin(),
                 [](const Renderable &r) { return std::addressof(r); });

  const auto RSM = _addReflectiveShadowMapPass(fg, std::move(lightView),
                                               light.color * light.intensity,
                                               std::move(visibleRenderables));
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

      builder.setSideEffect();
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
        for (uint32_t unit{kFirstFreeTextureBinding};
             const auto &[_, texture] : material.getDefaultTextures()) {
          rc.bindTexture(unit++, *texture);
        }
        _setTransform(lightViewProjection, modelMatrix);

        rc.setUniformVec3("u_LightIntensity", lightIntensity)
          .draw(*mesh.vertexBuffer, *mesh.indexBuffer,
                mesh.subMeshes[subMeshId].geometryInfo);
      }
      rc.endRendering(framebuffer);
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
