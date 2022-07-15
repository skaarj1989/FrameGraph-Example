#pragma once

#include "fg/Fwd.hpp"
#include "Passes/BaseGeometryPass.hpp"
#include "Light.hpp"
#include "ReflectiveShadowMapData.hpp"
#include "LightPropagationVolumesData.hpp"
#include "Grid.hpp"
#include <span>

class GlobalIllumination final : public BaseGeometryPass {
public:
  explicit GlobalIllumination(RenderContext &);
  ~GlobalIllumination();

  void update(FrameGraph &, FrameGraphBlackboard &, const Grid &,
              const PerspectiveCamera &, const Light &,
              std::span<const Renderable>, uint32_t numPropagations);

  [[nodiscard]] FrameGraphResource
  addDebugPass(FrameGraph &, FrameGraphBlackboard &, const Grid &,
               const PerspectiveCamera &, FrameGraphResource target);

private:
  [[nodiscard]] ReflectiveShadowMapData _addReflectiveShadowMapPass(
    FrameGraph &, const glm::mat4 &lightViewProjection,
    glm::vec3 lightIntensity, std::vector<const Renderable *> &&);

  [[nodiscard]] LightPropagationVolumesData
  _addRadianceInjectionPass(FrameGraph &, const ReflectiveShadowMapData &,
                            const Grid &);
  LightPropagationVolumesData
  _addRadiancePropagationPass(FrameGraph &, const LightPropagationVolumesData &,
                              const Grid &, uint32_t iteration);

  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) override;

private:
  GraphicsPipeline m_radianceInjectionPipeline;
  GraphicsPipeline m_radiancePropagationPipeline;

  GraphicsPipeline m_debugPipeline;
};
