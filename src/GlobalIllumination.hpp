#pragma once

#include "fg/Fwd.hpp"
#include "Passes/BaseGeometryPass.hpp"
#include "Light.hpp"
#include "ReflectiveShadowMapData.hpp"
#include "LightPropagationVolumesData.hpp"

#include <span>

class GlobalIllumination final : public BaseGeometryPass {
public:
  explicit GlobalIllumination(RenderContext &);
  ~GlobalIllumination();

  void update(FrameGraph &, const PerspectiveCamera &, const Light &,
              std::span<const Renderable>);

private:
  [[nodiscard]] ReflectiveShadowMapData _addReflectiveShadowMapPass(
    FrameGraph &, const glm::mat4 &lightViewProjection,
    glm::vec3 lightIntensity, std::vector<const Renderable *> &&);

  GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                           const Material *) override;
};
