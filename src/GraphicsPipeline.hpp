#pragma once

#include "TypeDefs.hpp"
#include "glm/vec4.hpp"
#include <array>
#include <optional>

struct DepthStencilState {
  bool depthTest{false};
  bool depthWrite{true};
  CompareOp depthCompareOp{CompareOp::Less};

  auto operator<=>(const DepthStencilState &) const = default;
};

enum class BlendOp : GLenum {
  Add = GL_FUNC_ADD,
  Subtract = GL_FUNC_SUBTRACT,
  ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
  Min = GL_MIN,
  Max = GL_MAX
};
enum class BlendFactor : GLenum {
  Zero = GL_ZERO,
  One = GL_ONE,
  SrcColor = GL_SRC_COLOR,
  OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
  DstColor = GL_DST_COLOR,
  OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
  SrcAlpha = GL_SRC_ALPHA,
  OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
  DstAlpha = GL_DST_ALPHA,
  OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
  ConstantColor = GL_CONSTANT_COLOR,
  OneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
  ConstantAlpha = GL_CONSTANT_ALPHA,
  OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
  SrcAlphaSaturate = GL_SRC_ALPHA_SATURATE,
  Src1Color = GL_SRC1_COLOR,
  OneMinusSrc1Color = GL_ONE_MINUS_SRC1_COLOR,
  Src1Alpha = GL_SRC1_ALPHA,
  OneMinusSrc1Alpha = GL_ONE_MINUS_SRC1_ALPHA
};

// src = incoming values
// dest = values that are already in a framebuffer
struct BlendState {
  bool enabled{false};

  BlendFactor srcColor{BlendFactor::One};
  BlendFactor destColor{BlendFactor::Zero};
  BlendOp colorOp{BlendOp::Add};

  BlendFactor srcAlpha{BlendFactor::One};
  BlendFactor destAlpha{BlendFactor::Zero};
  BlendOp alphaOp{BlendOp::Add};

  auto operator<=>(const BlendState &) const = default;
};
constexpr auto kMaxNumBlendStates = 2;

enum class PolygonMode : GLenum {
  Point = GL_POINT,
  Line = GL_LINE,
  Fill = GL_FILL
};
enum class CullMode : GLenum {
  None = GL_NONE,
  Back = GL_BACK,
  Front = GL_FRONT
};
struct PolygonOffset {
  float factor{0.0f}, units{0.0f};

  auto operator<=>(const PolygonOffset &) const = default;
};

struct RasterizerState {
  PolygonMode polygonMode{PolygonMode::Fill};
  CullMode cullMode{CullMode::Back};
  std::optional<PolygonOffset> polygonOffset;
  bool depthClampEnable{false};
  bool scissorTest{false};

  auto operator<=>(const RasterizerState &) const = default;
};

class GraphicsPipeline {
  friend class RenderContext;

public:
  GraphicsPipeline() = default;
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline(GraphicsPipeline &&) noexcept;
  ~GraphicsPipeline();

  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(GraphicsPipeline &&) noexcept;

  class Builder {
  public:
    Builder() = default;
    Builder(const Builder &) = delete;
    Builder(Builder &&) noexcept = delete;
    ~Builder() = default;

    Builder &operator=(const Builder &) = delete;
    Builder &operator=(Builder &&) noexcept = delete;

    Builder &setShaderProgram(GLuint program);
    Builder &setVertexArray(GLuint vao);

    Builder &setDepthStencil(const DepthStencilState &);
    Builder &setRasterizerState(const RasterizerState &);
    Builder &setBlendState(uint32_t attachment, const BlendState &);

    [[nodiscard]] GraphicsPipeline build();

  private:
    GLuint m_vertexArray{GL_NONE};
    GLuint m_program{GL_NONE};

    DepthStencilState m_depthStencilState{};
    RasterizerState m_rasterizerState{};
    std::array<BlendState, kMaxNumBlendStates> m_blendStates{};
  };

private:
  Rect2D m_viewport, m_scissor;

  GLuint m_program{GL_NONE};
  GLuint m_vertexArray{GL_NONE};

  DepthStencilState m_depthStencilState{};
  RasterizerState m_rasterizerState{};
  std::array<BlendState, kMaxNumBlendStates> m_blendStates{};
};
