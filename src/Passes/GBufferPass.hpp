#pragma once

#include "fg/Fwd.hpp"
#include "BaseGeometryPass.hpp"
#include <span>

class GBufferPass final : public BaseGeometryPass {
public:
  explicit GBufferPass(RenderContext &);
  ~GBufferPass() = default;

  void addGeometryPass(FrameGraph &, FrameGraphBlackboard &,
                       Extent2D resolution, const PerspectiveCamera &,
                       std::span<const Renderable *>);

private:
  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) final;
};
