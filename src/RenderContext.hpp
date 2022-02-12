#pragma once

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Texture.hpp"
#include "VertexAttributes.hpp"
#include "GraphicsPipeline.hpp"
#include "glm/glm.hpp"
#include <variant>
#include <string_view>
#include <unordered_map>

using UniformBuffer = Buffer;
using StorageBuffer = Buffer;

struct ImageData {
  GLenum format{GL_NONE};
  GLenum dataType{GL_NONE};
  const void *pixels{nullptr};
};

template <typename T>
using OptionalReference = std::optional<std::reference_wrapper<T>>;

using ClearValue = std::variant<glm::vec4, float>;

struct AttachmentInfo {
  Texture &image;
  int32_t mipLevel{0};
  std::optional<int32_t> layer{};
  std::optional<ClearValue> clearValue{};
};
struct RenderingInfo {
  Rect2D area;
  std::vector<AttachmentInfo> colorAttachments;
  std::optional<AttachmentInfo> depthAttachment{};
};

enum class PrimitiveTopology : GLenum {
  Undefined = GL_NONE,

  PointList = GL_POINTS,
  LineList = GL_LINES,
  LineStrip = GL_LINE_STRIP,

  TriangleList = GL_TRIANGLES,
  TriangleStrip = GL_TRIANGLE_STRIP,

  PatchList = GL_PATCHES
};

struct GeometryInfo {
  PrimitiveTopology topology{PrimitiveTopology::TriangleList};
  uint32_t vertexOffset{0};
  uint32_t numVertices{0};
  uint32_t indexOffset{0};
  uint32_t numIndices{0};

  auto operator<=>(const GeometryInfo &) const = default;
};

class RenderContext {
public:
  RenderContext();
  RenderContext(const RenderContext &) = delete;
  RenderContext(RenderContext &&) noexcept = delete;
  ~RenderContext();

  RenderContext &operator=(const RenderContext &) = delete;
  RenderContext &operator=(RenderContext &&) noexcept = delete;

  // ---

  [[nodiscard]] Buffer createBuffer(GLsizeiptr size,
                                    const void *data = nullptr);
  [[nodiscard]] VertexBuffer createVertexBuffer(GLsizei stride,
                                                int64_t capacity,
                                                const void *data = nullptr);
  [[nodiscard]] IndexBuffer createIndexBuffer(IndexType, int64_t capacity,
                                              const void *data = nullptr);

  [[nodiscard]] GLuint getVertexArray(const VertexAttributes &);

  [[nodiscard]] GLuint createGraphicsProgram(const std::string_view vertCode,
                                             const std::string_view fragCode);
  [[nodiscard]] GLuint createComputeProgram(const std::string_view code);

  [[nodiscard]] Texture createTexture2D(Extent2D extent, PixelFormat,
                                        uint32_t numMipLevels = 1u,
                                        uint32_t numLayers = 0u);
  [[nodiscard]] Texture createCubemap(uint32_t size, PixelFormat,
                                      uint32_t numMipLevels = 1u,
                                      uint32_t numLayers = 0u);

  RenderContext &generateMipmaps(Texture &);

  RenderContext &setupSampler(Texture &, const SamplerInfo &);
  [[nodiscard]] GLuint createSampler(const SamplerInfo &);

  RenderContext &clear(Texture &);
  // Upload Texture2D
  RenderContext &upload(Texture &, GLint mipLevel, glm::uvec2 dimensions,
                        const ImageData &);
  // Upload Cubemap face
  RenderContext &upload(Texture &, GLint mipLevel, GLint face,
                        glm::uvec2 dimensions, const ImageData &);
  RenderContext &upload(Texture &, GLint mipLevel, const glm::uvec3 &dimensions,
                        GLint face, GLsizei layer, const ImageData &);

  RenderContext &clear(Buffer &);
  RenderContext &upload(Buffer &, GLintptr offset, GLsizeiptr size,
                        const void *data);
  [[nodiscard]] void *map(Buffer &);
  RenderContext &unmap(Buffer &);

  // ---

  RenderContext &destroy(Buffer &);
  RenderContext &destroy(Texture &);
  RenderContext &destroy(GraphicsPipeline &);

  // ---

  RenderContext &dispatch(GLuint computeProgram, const glm::uvec3 &numGroups);

  /*
   * @brief Offscreen rendering
   * @remark Don't use an rvalue, Visual Studio will throw ICE on you
   */
  [[nodiscard]] GLuint beginRendering(const RenderingInfo &);
  // @brief Draw to swapchain image
  RenderContext &beginRendering(const Rect2D &area,
                                std::optional<glm::vec4> clearColor = {},
                                std::optional<float> clearDepth = {},
                                std::optional<int> clearStencil = {});
  // @remark Must be called after offscreen rendering
  RenderContext &endRendering(GLuint framebuffer);

  RenderContext &setGraphicsPipeline(const GraphicsPipeline &);

  // ---

  RenderContext &setUniform1f(const std::string_view name, float);
  RenderContext &setUniform1i(const std::string_view name, int32_t);
  RenderContext &setUniform1ui(const std::string_view name, uint32_t);

  RenderContext &setUniformVec3(const std::string_view name, const glm::vec3 &);
  RenderContext &setUniformVec4(const std::string_view name, const glm::vec4 &);

  RenderContext &setUniformMat3(const std::string_view name, const glm::mat3 &);
  RenderContext &setUniformMat4(const std::string_view name, const glm::mat4 &);

  // ---

  RenderContext &setViewport(const Rect2D &);
  RenderContext &setScissor(const Rect2D &);

  // ---

  RenderContext &bindImage(GLuint unit, const Texture &, GLint mipLevel,
                           GLenum access);
  RenderContext &bindTexture(GLuint unit, const Texture &,
                             std::optional<GLuint> samplerId = {});
  RenderContext &bindUniformBuffer(GLuint index, const UniformBuffer &);
  RenderContext &bindStorageBuffer(GLuint index, const StorageBuffer &);

  RenderContext &drawFullScreenTriangle();
  RenderContext &drawCube();

  RenderContext &draw(OptionalReference<const VertexBuffer>,
                      OptionalReference<const IndexBuffer>,
                      const GeometryInfo &, uint32_t numInstances = 1);

private:
  void _setupDebugCallback();
  static void _debugCallback(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length,
                             const GLchar *message, const void *userData);

  [[nodiscard]] GLuint _createVertexArray(const VertexAttributes &);

  [[nodiscard]] Texture _createImmutableTexture(Extent2D, uint32_t depth,
                                                PixelFormat, uint32_t numFaces,
                                                uint32_t numMipLevels,
                                                uint32_t numLayers);

  [[nodiscard]] GLuint _createShader(GLenum type, const std::string_view code);
  [[nodiscard]] GLuint
  _createShaderProgram(std::initializer_list<GLuint> shaders);

  void _setShaderProgram(GLuint);
  void _setVertexArray(GLuint);
  void _setVertexBuffer(const VertexBuffer &);
  void _setIndexBuffer(const IndexBuffer &);

  void _setDepthTest(bool enabled, CompareOp);
  void _setDepthWrite(bool enabled);

  void _setPolygonMode(PolygonMode);
  void _setPolygonOffset(std::optional<PolygonOffset>);
  void _setCullMode(CullMode);
  void _setDepthClamp(bool enabled);
  void _setLineWidth(float);
  void _setScissorTest(bool enabled);

  void _setBlendState(GLuint index, const BlendState &);

private:
  GLuint m_dummyVAO{GL_NONE};
  std::unordered_map<std::size_t, GLuint> m_vertexArrays;

  GraphicsPipeline m_currentPipeline{};
  bool m_renderingStarted{false};
};

class DebugMarker {
public:
  explicit DebugMarker(const std::string_view name);
  ~DebugMarker();
};

#if 1
#  define NAMED_DEBUG_MARKER(name)                                             \
    const DebugMarker dm##__LINE__ { name }
#  define DEBUG_MARKER()                                                       \
    const DebugMarker dm##__LINE__ { __FUNCTION__ }
#else
#  define NAMED_DEBUG_MARKER(name)
#  define DEBUG_MARKER()
#endif
