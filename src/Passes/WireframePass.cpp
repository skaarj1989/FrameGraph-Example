#include "WireframePass.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../GBufferData.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "tracy/TracyOpenGL.hpp"

WireframePass::WireframePass(RenderContext &rc) : BaseGeometryPass{rc} {}

FrameGraphResource WireframePass::addGeometryPass(
  FrameGraph &fg, FrameGraphBlackboard &blackboard, FrameGraphResource target,
  const PerspectiveCamera &camera, std::span<const Renderable *> renderables) {
  const auto &gBuffer = blackboard.get<GBufferData>();

  fg.addCallbackPass(
    "Wireframe Pass",
    [&](FrameGraph::Builder &builder, auto &) {
      builder.read(gBuffer.depth);

      target = builder.write(target);
    },
    [=, camera = &camera](const auto &, FrameGraphPassResources &resources,
                          void *ctx) {
      NAMED_DEBUG_MARKER("WireframePass");
      TracyGpuZone("WireframePass");

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
      for (const auto *renderable : renderables) {
        const auto &[mesh, subMeshIndex, material, _0, modelMatrix, _1] =
          *renderable;

        rc.setGraphicsPipeline(_getPipeline(*mesh.vertexFormat, nullptr))
          .setUniformMat4("u_Transform.modelViewProjMatrix",
                          camera->getViewProjection() * modelMatrix)
          .draw(*mesh.vertexBuffer, *mesh.indexBuffer,
                mesh.subMeshes[subMeshIndex].geometryInfo);
      }
      rc.endRendering(framebuffer);
    });

  return target;
}

GraphicsPipeline
WireframePass::_createBasePassPipeline(const VertexFormat &vertexFormat,
                                       const Material *material) {
  assert(material == nullptr);

  const auto vao = m_renderContext.getVertexArray(vertexFormat.getAttributes());

  ShaderCodeBuilder shaderCodeBuilder;
  const auto program = m_renderContext.createGraphicsProgram(
    shaderCodeBuilder.build("Geometry.vert"),
    shaderCodeBuilder.build("FlatColor.frag"));

  return GraphicsPipeline::Builder{}
    .setDepthStencil({
      .depthTest = true,
      .depthWrite = false,
      .depthCompareOp = CompareOp::LessOrEqual,
    })
    .setRasterizerState({
      .polygonMode = PolygonMode::Line,
      .cullMode = CullMode::Back,
      .polygonOffset =
        PolygonOffset{
          .factor = 0.0f,
          .units = -1000.0f,
        },
      .scissorTest = false,
    })
    .setVertexArray(vao)
    .setShaderProgram(program)
    .build();
}
