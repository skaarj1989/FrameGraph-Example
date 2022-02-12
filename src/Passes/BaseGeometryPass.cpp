#include "BaseGeometryPass.hpp"
#include "../Hash.hpp"
#include "spdlog/spdlog.h"

#include <sstream>
#include <format>

namespace {

struct GPUTransform {
  GPUTransform(const PerspectiveCamera &camera, const glm::mat4 &modelMatrix)
      : modelView{camera.getView() * modelMatrix},
        normal{glm::transpose(glm::inverse(glm::mat3(modelView)))},
        modelViewProj{camera.getProjection() * modelView} {}

  const glm::mat4 modelView;
  const glm::mat4 normal;
  const glm::mat4 modelViewProj;
};

} // namespace

//
// BaseGeometryPass class:
//

BaseGeometryPass::BaseGeometryPass(RenderContext &rc) : m_renderContext{rc} {}
BaseGeometryPass::~BaseGeometryPass() {
  for (auto &[_, pipeline] : m_pipelines)
    m_renderContext.destroy(pipeline);
}

void BaseGeometryPass::_setTransform(const PerspectiveCamera &camera,
                                     const glm::mat4 &modelMatrix) {
  const GPUTransform xf{camera, modelMatrix};
  m_renderContext.setUniformMat4("u_Transform.modelView", xf.modelView)
    .setUniformMat4("u_Transform.normalMatrix", xf.normal)
    .setUniformMat4("u_Transform.modelViewProj", xf.modelViewProj);
}

GraphicsPipeline &
BaseGeometryPass::_getPipeline(const VertexFormat &vertexFormat,
                               const Material *material) {
  auto hash = vertexFormat.getHash();
  if (material) hashCombine(hash, material->getHash());

  GraphicsPipeline *basePassPipeline{nullptr};
  if (const auto it = m_pipelines.find(hash); it != m_pipelines.cend())
    basePassPipeline = &it->second;
  if (!basePassPipeline) {
    auto pipeline = _createBasePassPipeline(vertexFormat, material);
    SPDLOG_INFO("Created pipeline: {}", hash);
    const auto &it =
      m_pipelines.insert_or_assign(hash, std::move(pipeline)).first;
    basePassPipeline = &it->second;
  }
  return *basePassPipeline;
}

//
// Utility:
//

std::string getSamplersChunk(const TextureResources &textures,
                             uint32_t firstBinding) {
  std::stringstream samplersChunk;
  for (uint32_t lastBinding{firstBinding};
       const auto &[alias, texture] : textures) {
    std::ostream_iterator<std::string>{samplersChunk, "\n"} = std::format(
      R"(layout(binding = {}) uniform sampler2D {};)", lastBinding++, alias);
  }
  return samplersChunk.str();
}
