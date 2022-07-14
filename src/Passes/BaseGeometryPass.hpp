#pragma once

#include "../PerspectiveCamera.hpp"
#include "../Renderable.hpp"

class BaseGeometryPass {
public:
  explicit BaseGeometryPass(RenderContext &);
  virtual ~BaseGeometryPass();

protected:
  void _setTransform(const PerspectiveCamera &, const glm::mat4 &modelMatrix);
  void _setTransform(const glm::mat4 &viewProjection,
                     const glm::mat4 &modelMatrix);

  [[nodiscard]] GraphicsPipeline &_getPipeline(const VertexFormat &,
                                               const Material *);
  virtual GraphicsPipeline _createBasePassPipeline(const VertexFormat &,
                                                   const Material *) = 0;

protected:
  RenderContext &m_renderContext;
  std::unordered_map<std::size_t, GraphicsPipeline> m_pipelines;
};

[[nodiscard]] std::string getSamplersChunk(const TextureResources &,
                                           uint32_t firstBinding);
