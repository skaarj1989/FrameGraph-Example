#include "GraphicsPipeline.hpp"
#include "spdlog/spdlog.h"

//
// GraphicsPipeline class:
//

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&other) noexcept
    : m_program{other.m_program}, m_vertexArray{other.m_vertexArray},
      m_depthStencilState{std::move(other.m_depthStencilState)},
      m_rasterizerState{std::move(other.m_rasterizerState)},
      m_blendStates{std::move(other.m_blendStates)} {
  memset(&other, 0, sizeof(GraphicsPipeline));
}
GraphicsPipeline::~GraphicsPipeline() {
  if (m_program != GL_NONE) SPDLOG_WARN("ShaderProgram leak: {}", m_program);
}

GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&rhs) noexcept {
  if (this != &rhs) {
    m_vertexArray = rhs.m_vertexArray;
    m_program = rhs.m_program;
    m_depthStencilState = std::move(rhs.m_depthStencilState);
    m_rasterizerState = std::move(rhs.m_rasterizerState);
    m_blendStates = std::move(rhs.m_blendStates);

    memset(&rhs, 0, sizeof(GraphicsPipeline));
  }
  return *this;
}

//
// Builder:
//

using Builder = GraphicsPipeline::Builder;

Builder &GraphicsPipeline::Builder::setShaderProgram(GLuint program) {
  m_program = program;
  return *this;
}
Builder &GraphicsPipeline::Builder::setVertexArray(GLuint vao) {
  m_vertexArray = vao;
  return *this;
}

Builder &
GraphicsPipeline::Builder::setDepthStencil(const DepthStencilState &state) {
  m_depthStencilState = state;
  return *this;
}
Builder &
GraphicsPipeline::Builder::setRasterizerState(const RasterizerState &state) {
  m_rasterizerState = state;
  return *this;
}
Builder &GraphicsPipeline::Builder::setBlendState(uint32_t attachment,
                                                     const BlendState &state) {
  assert(attachment < kMaxNumBlendStates);
  m_blendStates[attachment] = state;
  return *this;
}

GraphicsPipeline GraphicsPipeline::Builder::build() {
  assert(m_program != GL_NONE);

  GraphicsPipeline gp;
  gp.m_program = m_program;
  gp.m_vertexArray = m_vertexArray;
  gp.m_depthStencilState = m_depthStencilState;
  gp.m_rasterizerState = m_rasterizerState;
  gp.m_blendStates = m_blendStates;

  m_program = GL_NONE;
  return gp;
}
