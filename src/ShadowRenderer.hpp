#pragma once

#include "fg/FrameGraphResource.hpp"
#include "Passes/BaseGeometryPass.hpp"
#include "Light.hpp"
#include <span>

class FrameGraph;
class FrameGraphBlackboard;

class ShadowRenderer final : public BaseGeometryPass {
public:
  explicit ShadowRenderer(RenderContext &);
  ~ShadowRenderer();

  void buildCascadedShadowMaps(FrameGraph &, FrameGraphBlackboard &,
                               const PerspectiveCamera &, const Light *,
                               std::span<const Renderable>);

  [[nodiscard]] FrameGraphResource visualizeCascades(FrameGraph &,
                                                     FrameGraphBlackboard &,
                                                     FrameGraphResource target);

private:
  void _setupDebugPipeline();

  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) final;

  [[nodiscard]] FrameGraphResource
  _addCascadePass(FrameGraph &,
                  std::optional<FrameGraphResource> cascadedShadowMaps,
                  const glm::mat4 &lightViewProj,
                  std::vector<const Renderable *> &&, uint32_t cascadeIdx);

private:
  Buffer m_shadowMatrices;
  Texture m_dummyShadowMaps;

  GraphicsPipeline m_debugPipeline;
};
