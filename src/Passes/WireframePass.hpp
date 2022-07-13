#pragma once

#include "fg/Fwd.hpp"
#include "BaseGeometryPass.hpp"
#include <span>

class FrameGraph;
class FrameGraphBlackboard;

class WireframePass final : public BaseGeometryPass {
public:
  explicit WireframePass(RenderContext &);
  ~WireframePass() = default;

  [[nodiscard]] FrameGraphResource
  addGeometryPass(FrameGraph &, FrameGraphBlackboard &,
                  FrameGraphResource target, const PerspectiveCamera &,
                  std::span<const Renderable *>);

private:
  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) final;
};
