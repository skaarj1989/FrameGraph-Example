#include "WorldRenderer.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "FrameGraphHelper.hpp"

#include "FrameData.hpp"
#include "BRDF.hpp"
#include "GlobalLightProbeData.hpp"
#include "GBufferData.hpp"
#include "WeightedBlendedData.hpp"
#include "LightsData.hpp"
#include "LightCullingData.hpp"
#include "BrightColorData.hpp"
#include "SSAOData.hpp"
#include "ReflectionsData.hpp"
#include "SceneColorData.hpp"

#include "ShaderCodeBuilder.hpp"

#include "LightUtility.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

#include <ranges>
#include <fstream>

namespace {

constexpr auto kMaxNumLights = 1024u;
constexpr auto kTileSize = 16u;

__declspec(align(16)) struct GPUCamera {
  GPUCamera(const PerspectiveCamera &camera)
      : projection{camera.getProjection()},
        inversedProjection{glm::inverse(projection)}, view{camera.getView()},
        inversedView{glm::inverse(view)}, fov{camera.getFov()},
        near{camera.getNear()}, far{camera.getFar()} {}

private:
  glm::mat4 projection{1.0f};
  glm::mat4 inversedProjection{1.0f};
  glm::mat4 view{1.0f};
  glm::mat4 inversedView{1.0f};
  float fov;
  float near, far;
  // Implicit padding, 4bytes
};
struct GPUFrameBlock {
  float time{0.0f};
  float deltaTime{0.0f};
  Extent2D resolution{0};
  GPUCamera camera;
  uint32_t renderFeatures{0};
};

// - Position and direction in view-space
// - Angles in radians
__declspec(align(16)) struct GPULight {
  GPULight(const Light &light, const glm::mat4 &view) {
    const glm::vec3 viewSpacePosition{view * glm::vec4{light.position, 1.0f}};

    position = glm::vec4{viewSpacePosition, light.range};
    color = glm::vec4{light.color, light.intensity};
    type = static_cast<uint32_t>(light.type);

    if (light.type == LightType::Spot) {
      innerConeAngle = glm::radians(light.innerConeAngle);
      outerConeAngle = glm::radians(light.outerConeAngle);
    }
    if (light.type != LightType::Point) {
      direction = glm::normalize(view * glm::vec4{light.direction, 0.0f});
    }
  }

private:
  glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};  // .w = range
  glm::vec4 direction{0.0f, 0.0f, 0.0f, 0.0f}; // .w = unused
  glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};     // .a = intensity
  uint32_t type{0};
  float innerConeAngle{1.0f};
  float outerConeAngle{1.0f};
  // Implicit padding, 4bytes
};

[[nodiscard]] bool isLightInFrustum(const Light &light,
                                    const Frustum &frustum) {
  switch (light.type) {
  case LightType::Point:
    return frustum.testSphere(toSphere(light));
  case LightType::Spot:
    return frustum.testCone(toCone(light));
  }
  return true; // Directional light, always visible
}
[[nodiscard]] auto getVisibleLights(std::span<const Light> lights,
                                    const Frustum &frustum) {
  std::vector<const Light *> visibleLights;
  visibleLights.reserve(lights.size());
  for (const auto &light : lights)
    if (isLightInFrustum(light, frustum)) visibleLights.push_back(&light);
  return visibleLights;
}

[[nodiscard]] const Light *
getFirstDirectionalLight(std::span<const Light *> lights) {
  const auto it =
    std::find_if(lights.begin(), lights.end(), [](const auto *light) {
      return light->type == LightType::Directional && light->castsShadow;
    });
  return it != lights.end() ? *it : nullptr;
}

[[nodiscard]] auto
getVisibleRenderables(std::span<const Renderable> renderables,
                      const PerspectiveCamera &camera) {
  std::vector<const Renderable *> result;
  for (const auto &renderable : renderables)
    if (camera.getFrustum().testAABB(renderable.aabb))
      result.push_back(&renderable);

  return result;
}

[[nodiscard]] std::vector<const Renderable *>
filterRenderables(std::span<const Renderable *> src, auto &&predicate) {
  auto out = src | std::views::filter(predicate);
  return {out.begin(), out.end()};
}

void importLightProbe(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                      LightProbe &lightProbe) {
  auto &globalLightProbe = blackboard.add<GlobalLightProbeData>();
  globalLightProbe.diffuse =
    importTexture(fg, "DiffuseIBL", &lightProbe.diffuse);
  globalLightProbe.specular =
    importTexture(fg, "SpecularIBL", &lightProbe.specular);
}

void uploadLights(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                  const PerspectiveCamera &camera,
                  std::span<const Light *> lights) {
  blackboard.add<LightsData>() = fg.addCallbackPass<LightsData>(
    "UploadLights",
    [&](FrameGraph::Builder &builder, LightsData &data) {
      constexpr auto kBufferSize =
        sizeof(GPULight) * kMaxNumLights + sizeof(uint32_t);
      data.buffer =
        builder.create<FrameGraphBuffer>("LightsBuffer", {.size = kBufferSize});
      data.buffer = builder.write(data.buffer);
    },
    [=, camera = &camera](const LightsData &data,
                          FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("UploadLights");
      TracyGpuZone("UploadLights");

      std::vector<GPULight> gpuLights;
      gpuLights.reserve(lights.size());
      for (const auto *light : lights)
        gpuLights.emplace_back(GPULight{*light, camera->getView()});

      const auto numLights = static_cast<uint32_t>(gpuLights.size());
      constexpr auto kLightsNumberOffset = sizeof(GPULight) * kMaxNumLights;

      auto &buffer = getBuffer(resources, data.buffer);
      static_cast<RenderContext *>(ctx)
        ->upload(buffer, 0, sizeof(GPULight) * numLights, gpuLights.data())
        .upload(buffer, kLightsNumberOffset, sizeof(uint32_t), &numLights);
    });
}

enum class SortOrder { FrontToBack, BackToFront };
struct SortByDistance {
  SortByDistance(const PerspectiveCamera &camera, SortOrder order)
      : m_origin{camera.getPosition()}, m_order{order} {}

  bool operator()(const Renderable *a, const Renderable *b) const noexcept {
    const auto distanceA =
      glm::distance(m_origin, glm::vec3{a->modelMatrix[3]});
    const auto distanceB =
      glm::distance(m_origin, glm::vec3{b->modelMatrix[3]});

    return m_order == SortOrder::FrontToBack ? distanceB > distanceA
                                             : distanceA > distanceB;
  }

private:
  const glm::vec3 m_origin{0.0f};
  const SortOrder m_order;
};

} // namespace

//
// WorldRenderer class:
//

WorldRenderer::WorldRenderer(RenderContext &rc)
    : m_renderContext{rc}, m_ibl{rc}, m_transientResources{rc},
      m_tiledLighting{rc, kMaxNumLights, kTileSize}, m_shadowRenderer{rc},
      m_gBufferPass{rc}, m_deferredLightingPass{rc, kMaxNumLights, kTileSize},
      m_skyboxPass{rc}, m_weightedBlendedPass{rc, kMaxNumLights, kTileSize},
      m_transparencyCompositionPass{rc}, m_wireframePass{rc},
      m_brightPass{rc}, m_ssao{rc}, m_ssr{rc}, m_tonemapPass{rc}, m_fxaa{rc},
      m_gammaCorrectionPass{rc}, m_vignettePass{rc}, m_blur{rc} {
  m_brdf = m_ibl.generateBRDF();
  _setupPipelines();
}
WorldRenderer::~WorldRenderer() {
  m_renderContext.destroy(m_brdf)
    .destroy(m_globalLightProbe.diffuse)
    .destroy(m_globalLightProbe.specular)

    .destroy(m_blitPipeline)
    .destroy(m_additiveBlitPipeline);
}

void WorldRenderer::setSkybox(Texture &cubemap) {
  if (m_skybox == &cubemap) return;

  m_skybox = &cubemap;
  m_globalLightProbe = {};

  m_globalLightProbe.diffuse = m_ibl.generateIrradiance(*m_skybox);
  m_globalLightProbe.specular = m_ibl.prefilterEnvMap(*m_skybox);
}

void WorldRenderer::drawFrame(const RenderSettings &settings,
                              Extent2D resolution,
                              const PerspectiveCamera &camera,
                              std::span<const Light> lights,
                              std::span<const Renderable> renderables,
                              float deltaTime) {
  if (resolution.width == 0 || resolution.height == 0) return;

  FrameGraph fg;
  FrameGraphBlackboard blackboard;

  blackboard.add<BRDF>().lut = importTexture(fg, "BRDF LUT", &m_brdf);
  importLightProbe(fg, blackboard, m_globalLightProbe);

  // FrameBlock is persistently bound to Uniform binding at index 0
  _uploadFrameBlock(fg, blackboard,
                    {
                      .deltaTime = deltaTime,
                      .resolution = resolution,
                      .camera = camera,
                      .features = settings.renderFeatures,
                    });

  auto visibleLights = getVisibleLights(lights, camera.getFrustum());

  const bool hasShadows = settings.renderFeatures & RenderFeature_Shadows;
  m_shadowRenderer.buildCascadedShadowMaps(
    fg, blackboard, camera,
    hasShadows ? getFirstDirectionalLight(visibleLights) : nullptr,
    renderables);

  auto visibleRenderables = getVisibleRenderables(renderables, camera);

  auto opaqueRenderables = filterRenderables(visibleRenderables, isOpaque);
  std::sort(opaqueRenderables.begin(), opaqueRenderables.end(),
            SortByDistance{camera, SortOrder::FrontToBack});
  m_gBufferPass.addGeometryPass(fg, blackboard, resolution, camera,
                                opaqueRenderables);

  uploadLights(fg, blackboard, camera, visibleLights);
  // Requires depth buffer, must be executed AFTER GBufferPass.
  m_tiledLighting.cullLights(fg, blackboard);

  auto transparentRenderables =
    filterRenderables(visibleRenderables, isTransparent);
  m_weightedBlendedPass.addPass(fg, blackboard, camera, transparentRenderables);

  if (settings.renderFeatures & RenderFeature_SSAO) {
    m_ssao.addPass(fg, blackboard);
    auto &ssao = blackboard.get<SSAOData>().ssao;
    ssao = m_blur.addTwoPassGaussianBlur(fg, ssao, 1.0f);
  }

  auto &sceneColor = blackboard.add<SceneColorData>();
  sceneColor.hdr = m_deferredLightingPass.addPass(fg, blackboard);
  sceneColor.hdr =
    m_skyboxPass.addPass(fg, blackboard, sceneColor.hdr, m_skybox);

  sceneColor.hdr =
    m_transparencyCompositionPass.addPass(fg, blackboard, sceneColor.hdr);

  auto &brightColor = blackboard.add<BrightColorData>().brightColor;
  brightColor =
    m_brightPass.addPass(fg, sceneColor.hdr, settings.bloom.threshold);
  for (uint32_t i{0}; i < settings.bloom.numPasses; ++i) {
    brightColor =
      m_blur.addTwoPassGaussianBlur(fg, brightColor, settings.bloom.blurScale);
  }

  const auto reflections = m_ssr.addPass(fg, blackboard);

  if ((settings.renderFeatures & RenderFeature_SSR))
    sceneColor.hdr = _addColor(fg, sceneColor.hdr, reflections);
  if (settings.renderFeatures & RenderFeature_Bloom)
    sceneColor.hdr = _addColor(fg, sceneColor.hdr, brightColor);

  sceneColor.ldr = m_tonemapPass.addPass(fg, sceneColor.hdr, settings.tonemap);
  if (settings.debugFlags & DebugFlag_Wireframe) {
    sceneColor.ldr = m_wireframePass.addGeometryPass(
      fg, blackboard, sceneColor.ldr, camera, visibleRenderables);
  }
  const auto antiAliased = m_fxaa.addPass(fg, blackboard, sceneColor.ldr);
  if (settings.renderFeatures & RenderFeature_FXAA)
    sceneColor.ldr = antiAliased;

  sceneColor.ldr = m_gammaCorrectionPass.addPass(fg, sceneColor.ldr);

  if (hasShadows && (settings.debugFlags & DebugFlag_CascadeSplits))
    sceneColor.ldr =
      m_shadowRenderer.visualizeCascades(fg, blackboard, sceneColor.ldr);

  const auto afterVignette = m_vignettePass.addPass(fg, sceneColor.ldr);
  if (settings.renderFeatures & RenderFeature_Vignette)
    sceneColor.ldr = afterVignette;

  _present(fg, blackboard, settings.outputMode);
  {
    ZoneScopedN("CompileFrameGraph");
    fg.compile();
  }

#if _DEBUG
  constexpr auto kInterval = 5.0f;
  static float stopwatch{kInterval};
  if (stopwatch >= kInterval) {
    std::ofstream{"fg.dot"} << fg;
    stopwatch = 0.0f;
  }
  stopwatch += deltaTime;
#endif

  {
    TracyGpuZone("ExecuteFrameGraph");
    fg.execute(&m_renderContext, &m_transientResources);
  }

  m_time += deltaTime;
  m_transientResources.update(deltaTime);
}

void WorldRenderer::_setupPipelines() {
  GraphicsPipeline::Builder pipelineBuilder{};
  pipelineBuilder
    .setDepthStencil({
      .depthTest = false,
      .depthWrite = false,
    })
    .setRasterizerState({
      .polygonMode = PolygonMode::Fill,
      .cullMode = CullMode::Back,
      .scissorTest = false,
    });

  ShaderCodeBuilder shaderCodeBuilder;
  const auto fullScreenTriangleCodeVS =
    shaderCodeBuilder.build("FullScreenTriangle.vert");
  const auto blitCodeFS = shaderCodeBuilder.build("Blit.frag");

  auto program =
    m_renderContext.createGraphicsProgram(fullScreenTriangleCodeVS, blitCodeFS);
  m_blitPipeline = pipelineBuilder.setShaderProgram(program).build();

  program =
    m_renderContext.createGraphicsProgram(fullScreenTriangleCodeVS, blitCodeFS);
  m_additiveBlitPipeline = pipelineBuilder
                             .setBlendState(0,
                                            {
                                              .enabled = true,
                                              .srcColor = BlendFactor::One,
                                              .destColor = BlendFactor::One,
                                            })
                             .setShaderProgram(program)
                             .build();
}

void WorldRenderer::_uploadFrameBlock(FrameGraph &fg,
                                      FrameGraphBlackboard &blackboard,
                                      const FrameInfo &frameInfo) {
  blackboard.add<FrameData>() = fg.addCallbackPass<FrameData>(
    "UploadFrameBlock",
    [&](FrameGraph::Builder &builder, FrameData &data) {
      data.frameBlock = builder.create<FrameGraphBuffer>(
        "FrameBlock", {.size = sizeof(GPUFrameBlock)});
      data.frameBlock = builder.write(data.frameBlock);

      builder.setSideEffect();
    },
    [=](const FrameData &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("UploadFrameBlock");
      TracyGpuZone("UploadFrameBlock");

      const GPUFrameBlock frameBlock{
        .time = m_time,
        .deltaTime = frameInfo.deltaTime,
        .resolution = frameInfo.resolution,
        .camera = GPUCamera{frameInfo.camera},
        .renderFeatures = frameInfo.features,
      };
      static_cast<RenderContext *>(ctx)->upload(
        getBuffer(resources, data.frameBlock), 0, sizeof(GPUFrameBlock),
        &frameBlock);
    });
}

FrameGraphResource WorldRenderer::_addColor(FrameGraph &fg,
                                            FrameGraphResource target,
                                            FrameGraphResource source) {
  fg.addCallbackPass(
    "AddColor",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(source);

      target = builder.write(target);
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("AddColor");
      TracyGpuZone("AddColor");

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
      rc.setGraphicsPipeline(m_additiveBlitPipeline)
        .bindTexture(0, getTexture(resources, source))
        .setUniform1ui("u_Mode", 0)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return target;
}
void WorldRenderer::_present(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                             OutputMode outputMode) {
  const auto [frameBlock] = blackboard.get<FrameData>();

  enum Mode_ {
    Mode_Discard = -1,
    Mode_Default,
    Mode_LinearDepth,
    Mode_RedChannel,
    Mode_GreenChannel,
    Mode_BlueChannel,
    Mode_AlphaChannel,
    Mode_WorldSpaceNormals,
  } mode{Mode_Default};

  FrameGraphResource output{-1};
  switch (outputMode) {
    using enum OutputMode;

  case Depth:
    output = blackboard.get<GBufferData>().depth;
    mode = Mode_LinearDepth;
    break;
  case Emissive:
    output = blackboard.get<GBufferData>().emissive;
    break;
  case BaseColor:
    output = blackboard.get<GBufferData>().albedo;
    break;
  case Normal:
    output = blackboard.get<GBufferData>().normal;
    mode = Mode_WorldSpaceNormals;
    break;
  case Metallic:
    output = blackboard.get<GBufferData>().metallicRoughnessAO;
    mode = Mode_RedChannel;
    break;
  case Roughness:
    output = blackboard.get<GBufferData>().metallicRoughnessAO;
    mode = Mode_GreenChannel;
    break;
  case AmbientOcclusion:
    output = blackboard.get<GBufferData>().metallicRoughnessAO;
    mode = Mode_BlueChannel;
    break;

  case SSAO:
    output = blackboard.get<SSAOData>().ssao;
    mode = Mode_RedChannel;
    break;
  case BrightColor:
    output = blackboard.get<BrightColorData>().brightColor;
    break;
  case Reflections:
    output = blackboard.get<ReflectionsData>().reflections;
    break;

  case Accum:
    if (auto oit = blackboard.try_get<WeightedBlendedData>(); oit)
      output = oit->accum;
    break;
  case Reveal:
    if (auto oit = blackboard.try_get<WeightedBlendedData>(); oit) {
      output = oit->reveal;
      mode = Mode_RedChannel;
    }
    break;

  case LightHeatmap: {
    const auto &lightCullingData = blackboard.get<LightCullingData>();
    if (lightCullingData.debugMap.has_value())
      output = *lightCullingData.debugMap;
  } break;

  case HDR:
    output = blackboard.get<SceneColorData>().hdr;
    break;
  case FinalImage:
    output = blackboard.get<SceneColorData>().ldr;
    break;

  default:
    assert(false);
    break;
  }
  if (output == -1) mode = Mode_Discard;

  fg.addCallbackPass(
    "Present",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(frameBlock);
      if (mode != Mode_Discard) builder.read(output);
      builder.setSideEffect();
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("Present");
      TracyGpuZone("Present");

      const auto extent =
        resources.getDescriptor<FrameGraphTexture>(output).extent;
      auto &rc = *static_cast<RenderContext *>(ctx);
      rc.beginRendering({.extent = extent}, glm::vec4{0.0f});
      if (mode != Mode_Discard) {
        rc.setGraphicsPipeline(m_blitPipeline)
          .bindUniformBuffer(0, getBuffer(resources, frameBlock))
          .bindTexture(0, getTexture(resources, output))
          .setUniform1ui("u_Mode", mode)
          .drawFullScreenTriangle();
      }
    });
}
