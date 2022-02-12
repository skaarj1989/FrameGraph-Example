#include "TiledLighting.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "FrameGraphHelper.hpp"
#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"

#include "GBufferData.hpp"
#include "LightsData.hpp"
#include "LightCullingData.hpp"

#include "ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

namespace {

constexpr auto kUseDebugOutput = true;

struct GPUFrustumTile {
  glm::vec4 planes[4];
};
static_assert(sizeof(GPUFrustumTile) == 64);

} // namespace

//
// TiledLighting class:
//

TiledLighting::TiledLighting(RenderContext &rc, uint32_t maxNumLights,
                             uint32_t tileSize)
    : m_renderContext{rc}, m_maxNumLights{maxNumLights}, m_tileSize{tileSize} {
  _setupPipelines();
}
TiledLighting::~TiledLighting() {
  glDeleteProgram(m_buildFrustumsProgram);
  glDeleteProgram(m_cullLightsProgram);
}

void TiledLighting::cullLights(FrameGraph &fg,
                               FrameGraphBlackboard &blackboard) {
  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto extent = fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;

  const auto f = static_cast<float>(m_tileSize);
  const auto numGroups = glm::ceil(glm::vec2{extent.width, extent.height} / f);
  const auto numFrustums = static_cast<uint32_t>(numGroups.x * numGroups.y);

  const auto gridFrustums =
    _buildFrustums(fg, numFrustums, glm::ceil(numGroups / f));
  _cullLights(fg, blackboard, gridFrustums, numFrustums, numGroups);
}

void TiledLighting::_setupPipelines() {
  ShaderCodeBuilder shaderCodeBuilder;
  shaderCodeBuilder.addDefine("TILE_SIZE", m_tileSize);

  m_buildFrustumsProgram = m_renderContext.createComputeProgram(
    shaderCodeBuilder.build("LightCulling/BuildFrustums.comp"));

  m_cullLightsProgram = m_renderContext.createComputeProgram(
    shaderCodeBuilder
      .addDefine("MAX_NUM_LIGHTS", m_maxNumLights)
      // 0 = Disabled, 1 = Heatmap, 2 = Depth
      .addDefine("_DEBUG_OUTPUT", kUseDebugOutput ? 1 : 0)
      .build("LightCulling/CullLights.comp"));
}

FrameGraphResource TiledLighting::_buildFrustums(FrameGraph &fg,
                                                 uint32_t numFrustums,
                                                 glm::uvec2 numGroups) {
  struct FrustumsData {
    FrameGraphResource gridFrustums;
  };
  auto &pass = fg.addCallbackPass<FrustumsData>(
    "BuildFrustums",
    [&](FrameGraph::Builder &builder, FrustumsData &data) {
      const auto bufferSize =
        static_cast<GLsizeiptr>(sizeof(GPUFrustumTile) * numFrustums);
      data.gridFrustums =
        builder.create<FrameGraphBuffer>("GridFrustums", {.size = bufferSize});
      data.gridFrustums = builder.write(data.gridFrustums);
    },
    [=](const FrustumsData &data, FrameGraphPassResources &resources,
        void *ctx) {
      NAMED_DEBUG_MARKER("BuildFrustums");
      TracyGpuZone("BuildFrustums");

      static_cast<RenderContext *>(ctx)
        ->bindStorageBuffer(0, getBuffer(resources, data.gridFrustums))
        .dispatch(m_buildFrustumsProgram, {numGroups, 1u});
    });

  return pass.gridFrustums;
}

void TiledLighting::_cullLights(FrameGraph &fg,
                                FrameGraphBlackboard &blackboard,
                                FrameGraphResource gridFrustums,
                                uint32_t numFrustums, glm::uvec2 numThreads) {
  const auto &gBuffer = blackboard.get<GBufferData>();
  const auto lightBuffer = blackboard.get<LightsData>().buffer;

  struct Data {
    FrameGraphResource lightsCounter;
    FrameGraphResource lightIndices;
    FrameGraphResource lightGrid;
    std::optional<FrameGraphResource> debugMap;
  };
  auto &pass = fg.addCallbackPass<Data>(
    "CullLights",
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(gridFrustums);
      builder.read(gBuffer.depth);
      builder.read(lightBuffer);

      data.lightsCounter = builder.create<FrameGraphBuffer>(
        "LightsCounter", {.size = sizeof(glm::uvec2)});
      data.lightsCounter = builder.write(data.lightsCounter);

      data.lightGrid = builder.create<FrameGraphTexture>(
        "LightGrid", {
                       .extent = {numThreads.x, numThreads.y},
                       .format = PixelFormat::RGBA32UI,
                     });
      data.lightGrid = builder.write(data.lightGrid);

      const auto averageOverlappingLightsPerTile = m_tileSize * m_tileSize;
      const auto bufferSize = static_cast<GLsizeiptr>(
        sizeof(glm::uvec2) * numFrustums * averageOverlappingLightsPerTile);
      data.lightIndices =
        builder.create<FrameGraphBuffer>("LightIndices", {.size = bufferSize});
      data.lightIndices = builder.write(data.lightIndices);

      if constexpr (kUseDebugOutput) {
        const auto extent =
          fg.getDescriptor<FrameGraphTexture>(gBuffer.depth).extent;

        data.debugMap = builder.create<FrameGraphTexture>(
          "DebugMap", {.extent = extent, .format = PixelFormat::RGBA8_UNorm});
        data.debugMap = builder.write(*data.debugMap);
      }
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("CullLights");
      TracyGpuZone("CullLights");

      auto &lightsCounterSSBO = getBuffer(resources, data.lightsCounter);

      auto &rc = *static_cast<RenderContext *>(ctx);
      rc.bindTexture(0, getTexture(resources, gBuffer.depth))

        .bindStorageBuffer(0, getBuffer(resources, lightBuffer))
        .bindStorageBuffer(1, getBuffer(resources, gridFrustums))

        .clear(lightsCounterSSBO)
        .bindStorageBuffer(2, lightsCounterSSBO)
        .bindStorageBuffer(3, getBuffer(resources, data.lightIndices))

        .bindImage(0, getTexture(resources, data.lightGrid), 0, GL_WRITE_ONLY);

      if constexpr (kUseDebugOutput) {
        rc.bindImage(1, getTexture(resources, *data.debugMap), 0,
                     GL_WRITE_ONLY);
      }

      rc.dispatch(m_cullLightsProgram, {numThreads, 1u});
    });

  blackboard.add<LightCullingData>() = {
    .lightIndices = pass.lightIndices,
    .lightGrid = pass.lightGrid,
    .debugMap = pass.debugMap,
  };
}
