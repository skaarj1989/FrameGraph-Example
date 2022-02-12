#pragma once

#include "BaseGeometryPass.hpp"
#include <span>

class FrameGraph;
class FrameGraphBlackboard;

class WeightedBlendedPass final : public BaseGeometryPass {
public:
  WeightedBlendedPass(RenderContext &, uint32_t maxNumLights,
                      uint32_t tileSize);
  ~WeightedBlendedPass() = default;

  void addPass(FrameGraph &, FrameGraphBlackboard &, const PerspectiveCamera &,
               std::span<const Renderable *>);

private:
  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) final;

private:
  const uint32_t m_maxNumLights, m_tileSize;
};
