#include "FinalPass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

#include "../FrameData.hpp"
#include "../GBufferData.hpp"
#include "../WeightedBlendedData.hpp"
#include "../SSAOData.hpp"
#include "../ReflectionsData.hpp"
#include "../LightCullingData.hpp"
#include "../BrightColorData.hpp"
#include "../SceneColorData.hpp"

#include "../FrameGraphHelper.hpp"
#include "../FrameGraphBuffer.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

FinalPass::FinalPass(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;

  auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("FullScreenTriangle.vert"),
    shaderCodeBuilder.build("FinalPass.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setShaderProgram(program)
                 .setDepthStencil({
                   .depthTest = false,
                   .depthWrite = false,
                 })
                 .setRasterizerState({
                   .polygonMode = PolygonMode::Fill,
                   .cullMode = CullMode::Back,
                   .scissorTest = false,
                 })
                 .build();
}
FinalPass::~FinalPass() { m_renderContext.destroy(m_pipeline); }

void FinalPass::compose(FrameGraph &fg, const FrameGraphBlackboard &blackboard,
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
    Mode_ViewSpaceNormals,
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
    // mode = Mode_ViewSpaceNormals;
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
    "FinalComposition",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(frameBlock);
      if (mode != Mode_Discard) builder.read(output);
      builder.setSideEffect();
    },
    [=](const auto &, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("FinalComposition");
      TracyGpuZone("FinalComposition");

      const auto extent =
        resources.getDescriptor<FrameGraphTexture>(output).extent;
      auto &rc = *static_cast<RenderContext *>(ctx);
      rc.beginRendering({.extent = extent}, glm::vec4{0.0f});
      if (mode != Mode_Discard) {
        rc.setGraphicsPipeline(m_pipeline)
          .bindUniformBuffer(0, getBuffer(resources, frameBlock))
          .bindTexture(0, getTexture(resources, output))
          .setUniform1ui("u_Mode", mode)
          .drawFullScreenTriangle();
      }
    });
}
