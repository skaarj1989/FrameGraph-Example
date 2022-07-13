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

#include "UploadFrameBlock.hpp"
#include "UploadLights.hpp"

#include "ShaderCodeBuilder.hpp"

#include "LightUtility.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

#include <ranges>
#include <fstream>

namespace {

constexpr auto kTileSize = 16u;

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
      m_tiledLighting{rc, kTileSize}, m_shadowRenderer{rc}, m_gBufferPass{rc},
      m_deferredLightingPass{rc, kTileSize}, m_skyboxPass{rc},
      m_weightedBlendedPass{rc, kTileSize}, m_transparencyCompositionPass{rc},
      m_wireframePass{rc}, m_brightPass{rc}, m_ssao{rc}, m_ssr{rc},
      m_tonemapPass{rc}, m_fxaa{rc}, m_gammaCorrectionPass{rc},
      m_vignettePass{rc}, m_blur{rc}, m_blit{rc}, m_finalPass{rc} {
  m_brdf = m_ibl.generateBRDF();
}
WorldRenderer::~WorldRenderer() {
  m_renderContext.destroy(m_brdf)
    .destroy(m_globalLightProbe.diffuse)
    .destroy(m_globalLightProbe.specular);
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

  uploadFrameBlock(fg, blackboard,
                   {
                     .time = m_time,
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

  uploadLights(fg, blackboard, std::move(visibleLights));
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
    sceneColor.hdr = m_blit.addColor(fg, sceneColor.hdr, reflections);
  if (settings.renderFeatures & RenderFeature_Bloom)
    sceneColor.hdr = m_blit.addColor(fg, sceneColor.hdr, brightColor);

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

  m_finalPass.compose(fg, blackboard, settings.outputMode);
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
