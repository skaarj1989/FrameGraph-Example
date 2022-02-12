#include "Blur.hpp"

#include "fg/FrameGraph.hpp"
#include "../FrameGraphHelper.hpp"
#include "../FrameGraphTexture.hpp"

#include "../ShaderCodeBuilder.hpp"

#include "TracyOpenGL.hpp"

Blur::Blur(RenderContext &rc) : m_renderContext{rc} {
  ShaderCodeBuilder shaderCodeBuilder;
  const auto program =
    rc.createGraphicsProgram(shaderCodeBuilder.build("FullScreenTriangle.vert"),
                             shaderCodeBuilder.build("Blur.frag"));

  m_pipeline = GraphicsPipeline::Builder{}
                 .setDepthStencil({
                   .depthTest = true,
                   .depthWrite = true,
                   .depthCompareOp = CompareOp::LessOrEqual,
                 })
                 .setRasterizerState({
                   .polygonMode = PolygonMode::Fill,
                   .cullMode = CullMode::Back,
                   .scissorTest = false,
                 })
                 .setShaderProgram(program)
                 .build();
}
Blur::~Blur() { m_renderContext.destroy(m_pipeline); }

FrameGraphResource Blur::addTwoPassGaussianBlur(FrameGraph &fg,
                                                FrameGraphResource input,
                                                float scale) {
  input = _addPass(fg, input, scale, false);
  return _addPass(fg, input, scale, true);
}

FrameGraphResource Blur::_addPass(FrameGraph &fg, FrameGraphResource input,
                                  float scale, bool horizontal) {
  const auto name =
    (horizontal ? "Horizontal" : "Vertical") + std::string{"Blur"};
  const auto &desc = fg.getDescriptor<FrameGraphTexture>(input);

  struct Data {
    FrameGraphResource output;
  };
  auto &pass = fg.addCallbackPass<Data>(
    name,
    [&](FrameGraph::Builder &builder, Data &data) {
      builder.read(input);

      data.output = builder.create<FrameGraphTexture>(
        "Blurred", {.extent = desc.extent, .format = desc.format});
      data.output = builder.write(data.output);
    },
    [=](const Data &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER(name);
      TracyGpuZone("Blur");

      const RenderingInfo renderingInfo{
        .area = {.extent = desc.extent},
        .colorAttachments = {{
          .image = getTexture(resources, data.output),
        }},
      };
      auto &rc = *static_cast<RenderContext *>(ctx);
      const auto framebuffer = rc.beginRendering(renderingInfo);
      rc.setGraphicsPipeline(m_pipeline)
        .bindTexture(0, getTexture(resources, input))
        .setUniform1f("u_Scale", scale)
        .setUniform1i("u_Horizontal", horizontal)
        .drawFullScreenTriangle()
        .endRendering(framebuffer);
    });

  return pass.output;
}
