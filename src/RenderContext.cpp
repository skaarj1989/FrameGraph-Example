#include "RenderContext.hpp"
#include "GLFW/glfw3.h"

#include "Hash.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "spdlog/spdlog.h"

#include <numeric>

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

namespace {

// @return {data type, number of components, normalize}
std::tuple<GLenum, GLint, GLboolean> statAttribute(VertexAttribute::Type type) {
  switch (type) {
    using enum VertexAttribute::Type;
  case Float:
    return {GL_FLOAT, 1, GL_FALSE};
  case Float2:
    return {GL_FLOAT, 2, GL_FALSE};
  case Float3:
    return {GL_FLOAT, 3, GL_FALSE};
  case Float4:
    return {GL_FLOAT, 4, GL_FALSE};

  case Int4:
    return {GL_INT, 4, GL_FALSE};

  case UByte4_Norm:
    return {GL_UNSIGNED_BYTE, 4, GL_TRUE};
  }
  return {GL_INVALID_INDEX, 0, GL_FALSE};
}

[[nodiscard]] GLenum selectTextureMinFilter(TexelFilter minFilter,
                                            MipmapMode mipmapMode) {
  GLenum result{GL_NONE};
  switch (minFilter) {
  case TexelFilter::Nearest:
    switch (mipmapMode) {
    case MipmapMode::None:
      result = GL_NEAREST;
      break;
    case MipmapMode::Nearest:
      result = GL_NEAREST_MIPMAP_NEAREST;
      break;
    case MipmapMode::Linear:
      result = GL_NEAREST_MIPMAP_LINEAR;
      break;
    }
    break;

  case TexelFilter::Linear:
    switch (mipmapMode) {
    case MipmapMode::None:
      result = GL_LINEAR;
      break;
    case MipmapMode::Nearest:
      result = GL_LINEAR_MIPMAP_NEAREST;
      break;
    case MipmapMode::Linear:
      result = GL_LINEAR_MIPMAP_LINEAR;
      break;
    }
    break;
  }
  assert(result != GL_NONE);
  return result;
}
[[nodiscard]] GLenum getIndexDataType(GLsizei stride) {
  switch (stride) {
  case sizeof(GLubyte):
    return GL_UNSIGNED_BYTE;
  case sizeof(GLushort):
    return GL_UNSIGNED_SHORT;
  case sizeof(GLuint):
    return GL_UNSIGNED_INT;
  }
  assert(false);
  return GL_NONE;
}

[[nodiscard]] GLenum getPolygonOffsetCap(PolygonMode polygonMode) {
  switch (polygonMode) {
  case PolygonMode::Fill:
    return GL_POLYGON_OFFSET_FILL;
  case PolygonMode::Line:
    return GL_POLYGON_OFFSET_LINE;
  case PolygonMode::Point:
    return GL_POLYGON_OFFSET_POINT;
  }
  assert(false);
  return GL_NONE;
}

} // namespace

//
// RenderContext class:
//

RenderContext::RenderContext() {
  if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
    throw std::runtime_error{"Failed to initialize GLAD"};
  }
  _setupDebugCallback();

#if GLM_CONFIG_CLIP_CONTROL & GLM_CLIP_CONTROL_ZO_BIT
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif
  glFrontFace(GL_CCW);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  glCreateVertexArrays(1, &m_dummyVAO);
}
RenderContext::~RenderContext() {
  glDeleteVertexArrays(1, &m_dummyVAO);
  for (auto [_, vao] : m_vertexArrays)
    glDeleteVertexArrays(1, &vao);

  m_currentPipeline = {};
}

Buffer RenderContext::createBuffer(GLsizeiptr size, const void *data) {
  GLuint buffer;
  glCreateBuffers(1, &buffer);
  glNamedBufferStorage(buffer, size, data,
                       data ? GL_NONE
                            : GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  return {buffer, size};
}
VertexBuffer RenderContext::createVertexBuffer(GLsizei stride, int64_t capacity,
                                               const void *data) {
  return VertexBuffer{createBuffer(stride * capacity, data), stride};
}
IndexBuffer RenderContext::createIndexBuffer(IndexType indexType,
                                             int64_t capacity,
                                             const void *data) {
  const auto stride = static_cast<GLsizei>(indexType);
  return IndexBuffer{createBuffer(stride * capacity, data), indexType};
}

GLuint RenderContext::getVertexArray(const VertexAttributes &attributes) {
  assert(!attributes.empty());

  std::size_t hash{0};
  for (const auto &[location, attribute] : attributes)
    hashCombine(hash, location, attribute);

  auto it = m_vertexArrays.find(hash);
  if (it == m_vertexArrays.cend()) {
    it = m_vertexArrays.emplace(hash, _createVertexArray(attributes)).first;
    SPDLOG_INFO("Created VAO: {}", hash);
  }
  return it->second;
}

GLuint RenderContext::createGraphicsProgram(const std::string_view vertCode,
                                            const std::string_view fragCode) {
  return _createShaderProgram({
    _createShader(GL_VERTEX_SHADER, vertCode),
    _createShader(GL_FRAGMENT_SHADER, fragCode),
  });
}
GLuint RenderContext::createComputeProgram(const std::string_view code) {
  return _createShaderProgram({
    _createShader(GL_COMPUTE_SHADER, code),
  });
}

Texture RenderContext::createTexture2D(Extent2D extent, PixelFormat pixelFormat,
                                       uint32_t numMipLevels,
                                       uint32_t numLayers) {
  assert(extent.width > 0 && extent.height > 0 and
         pixelFormat != PixelFormat::Unknown);

  if (numMipLevels <= 0)
    numMipLevels = calcMipLevels(glm::max(extent.width, extent.height));
  return _createImmutableTexture(extent, 0, pixelFormat, 1, numMipLevels,
                                 numLayers);
}
Texture RenderContext::createCubemap(uint32_t size, PixelFormat pixelFormat,
                                     uint32_t numMipLevels,
                                     uint32_t numLayers) {
  assert(size > 0 && pixelFormat != PixelFormat::Unknown);

  if (numMipLevels <= 0) numMipLevels = calcMipLevels(size);
  return _createImmutableTexture({size, size}, 0, pixelFormat, 6, numMipLevels,
                                 numLayers);
}

RenderContext &RenderContext::generateMipmaps(Texture &texture) {
  assert(texture);
  glGenerateTextureMipmap(texture.m_id);
  return *this;
}

RenderContext &RenderContext::setupSampler(Texture &texture,
                                           const SamplerInfo &samplerInfo) {
  assert(texture);

  glTextureParameteri(
    texture, GL_TEXTURE_MIN_FILTER,
    selectTextureMinFilter(samplerInfo.minFilter, samplerInfo.mipmapMode));
  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER,
                      static_cast<GLenum>(samplerInfo.magFilter));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S,
                      static_cast<GLenum>(samplerInfo.addressModeS));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T,
                      static_cast<GLenum>(samplerInfo.addressModeT));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_R,
                      static_cast<GLenum>(samplerInfo.addressModeR));
  if (samplerInfo.compareOp.has_value()) {
    glTextureParameteri(texture, GL_TEXTURE_COMPARE_MODE,
                        GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(texture, GL_TEXTURE_COMPARE_FUNC,
                        static_cast<GLenum>(*samplerInfo.compareOp));
  }
  glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR,
                       glm::value_ptr(samplerInfo.borderColor));
  return *this;
}
GLuint RenderContext::createSampler(const SamplerInfo &samplerInfo) {
  GLuint sampler{GL_NONE};
  glCreateSamplers(1, &sampler);

  glSamplerParameteri(
    sampler, GL_TEXTURE_MIN_FILTER,
    selectTextureMinFilter(samplerInfo.minFilter, samplerInfo.mipmapMode));
  glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER,
                      static_cast<GLenum>(samplerInfo.magFilter));
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S,
                      static_cast<GLenum>(samplerInfo.addressModeS));
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T,
                      static_cast<GLenum>(samplerInfo.addressModeT));
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R,
                      static_cast<GLenum>(samplerInfo.addressModeR));

  glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY,
                      samplerInfo.maxAnisotropy);

  if (samplerInfo.compareOp.has_value()) {
    glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE,
                        GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC,
                        static_cast<GLenum>(*samplerInfo.compareOp));
  }
  glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR,
                       glm::value_ptr(samplerInfo.borderColor));

  return sampler;
}

RenderContext &RenderContext::clear(Texture &texture) {
  assert(texture);
  uint8_t v{0};
  glClearTexImage(texture, 0, GL_RED, GL_UNSIGNED_BYTE, &v);
  return *this;
}
RenderContext &RenderContext::upload(Texture &texture, GLint mipLevel,
                                     glm::uvec2 dimensions,
                                     const ImageData &image) {
  return upload(texture, mipLevel, {dimensions, 0}, 0, 0, image);
}
RenderContext &RenderContext::upload(Texture &texture, GLint mipLevel,
                                     GLint face, glm::uvec2 dimensions,
                                     const ImageData &image) {
  return upload(texture, mipLevel, {dimensions, 0}, face, 0, image);
}
RenderContext &RenderContext::upload(Texture &texture, GLint mipLevel,
                                     const glm::uvec3 &dimensions, GLint face,
                                     GLsizei layer, const ImageData &image) {
  assert(texture && image.pixels != nullptr);

  switch (texture.m_type) {
  case GL_TEXTURE_1D:
    glTextureSubImage1D(texture.m_id, mipLevel, 0, dimensions.x, image.format,
                        image.dataType, image.pixels);
    break;
  case GL_TEXTURE_1D_ARRAY:
    glTextureSubImage2D(texture.m_id, mipLevel, 0, 0, dimensions.x, layer,
                        image.format, image.dataType, image.pixels);
    break;
  case GL_TEXTURE_2D:
    glTextureSubImage2D(texture.m_id, mipLevel, 0, 0, dimensions.x,
                        dimensions.y, image.format, image.dataType,
                        image.pixels);
    break;
  case GL_TEXTURE_2D_ARRAY:
    glTextureSubImage3D(texture.m_id, mipLevel, 0, 0, layer, dimensions.x,
                        dimensions.y, 1, image.format, image.dataType,
                        image.pixels);
    break;
  case GL_TEXTURE_3D:
    glTextureSubImage3D(texture.m_id, mipLevel, 0, 0, 0, dimensions.x,
                        dimensions.y, dimensions.z, image.format,
                        image.dataType, image.pixels);
    break;
  case GL_TEXTURE_CUBE_MAP:
    glTextureSubImage3D(texture.m_id, mipLevel, 0, 0, face, dimensions.x,
                        dimensions.y, 1, image.format, image.dataType,
                        image.pixels);
    break;
  case GL_TEXTURE_CUBE_MAP_ARRAY: {
    const auto zoffset = (layer * 6) + face; // desired layer-face
    // depth = how many layer-faces
    glTextureSubImage3D(texture.m_id, mipLevel, 0, 0, zoffset, dimensions.x,
                        dimensions.y, 6 * texture.m_numLayers, image.format,
                        image.dataType, image.pixels);
  } break;

  default:
    assert(false);
  }
  return *this;
}

RenderContext &RenderContext::clear(Buffer &buffer) {
  assert(buffer);
  uint8_t v{0};
  glClearNamedBufferData(buffer.m_id, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &v);
  return *this;
}
RenderContext &RenderContext::upload(Buffer &buffer, GLintptr offset,
                                     GLsizeiptr size, const void *data) {
  assert(buffer);
  if (size > 0 && data != nullptr)
    glNamedBufferSubData(buffer.m_id, offset, size, data);
  return *this;
}
void *RenderContext::map(Buffer &buffer) {
  assert(buffer);
  if (!buffer.isMapped())
    buffer.m_mappedMemory = glMapNamedBuffer(buffer, GL_WRITE_ONLY);
  return buffer.m_mappedMemory;
}
RenderContext &RenderContext::unmap(Buffer &buffer) {
  assert(buffer);
  if (buffer.isMapped()) {
    glUnmapNamedBuffer(buffer);
    buffer.m_mappedMemory = nullptr;
  }
  return *this;
}

RenderContext &RenderContext::destroy(Buffer &buffer) {
  if (buffer) {
    glDeleteBuffers(1, &buffer.m_id);
    buffer = {};
  }
  return *this;
}
RenderContext &RenderContext::destroy(Texture &texture) {
  if (texture) {
    glDeleteTextures(1, &texture.m_id);
    if (texture.m_view != GL_NONE) glDeleteTextures(1, &texture.m_view);
    texture = {};
  }
  return *this;
}
RenderContext &RenderContext::destroy(GraphicsPipeline &gp) {
  if (gp.m_program != GL_NONE) {
    glDeleteProgram(gp.m_program);
    gp.m_program = GL_NONE;
  }
  gp.m_vertexArray = GL_NONE;
  return *this;
}

RenderContext &RenderContext::dispatch(GLuint computeProgram,
                                       const glm::uvec3 &numGroups) {
  _setShaderProgram(computeProgram);
  glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);
  return *this;
}

GLuint RenderContext::beginRendering(const RenderingInfo &renderingInfo) {
  assert(!m_renderingStarted);

  TracyGpuZone("BeginRendering");

  GLuint framebuffer;
  glCreateFramebuffers(1, &framebuffer);
  if (renderingInfo.depthAttachment.has_value()) {
    _attachTexture(framebuffer, GL_DEPTH_ATTACHMENT,
                   *renderingInfo.depthAttachment);
  }
  for (uint8_t i{0}; i < renderingInfo.colorAttachments.size(); ++i) {
    _attachTexture(framebuffer, GL_COLOR_ATTACHMENT0 + i,
                   renderingInfo.colorAttachments[i]);
  }
  if (const auto n = renderingInfo.colorAttachments.size(); n > 0) {
    std::vector<GLenum> colorBuffers(n);
    std::iota(colorBuffers.begin(), colorBuffers.end(), GL_COLOR_ATTACHMENT0);
    glNamedFramebufferDrawBuffers(framebuffer, colorBuffers.size(),
                                  colorBuffers.data());
  }
#ifdef _DEBUG
  const auto status =
    glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
  assert(GL_FRAMEBUFFER_COMPLETE == status);
#endif

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
  setViewport(renderingInfo.area);
  _setScissorTest(false);

  if (renderingInfo.depthAttachment.has_value())
    if (renderingInfo.depthAttachment->clearValue.has_value()) {
      _setDepthWrite(true);

      const auto clearValue =
        std::get<float>(*renderingInfo.depthAttachment->clearValue);
      glClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &clearValue);
    }
  for (int32_t i{0}; const auto &attachment : renderingInfo.colorAttachments) {
    if (attachment.clearValue.has_value()) {
      const auto &clearValue = std::get<glm::vec4>(*attachment.clearValue);
      glClearNamedFramebufferfv(framebuffer, GL_COLOR, i,
                                glm::value_ptr(clearValue));
    }
    ++i;
  }

  m_renderingStarted = true;
  return framebuffer;
}
RenderContext &RenderContext::beginRendering(
  const Rect2D &area, std::optional<glm::vec4> clearColor,
  std::optional<float> clearDepth, std::optional<int> clearStencil) {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
  setViewport(area);
  _setScissorTest(false);

  if (clearDepth.has_value()) {
    _setDepthWrite(true);
    glClearNamedFramebufferfv(GL_NONE, GL_DEPTH, 0, &clearDepth.value());
  }
  if (clearStencil.has_value())
    glClearNamedFramebufferiv(GL_NONE, GL_STENCIL, 0, &clearStencil.value());
  if (clearColor.has_value()) {
    glClearNamedFramebufferfv(GL_NONE, GL_COLOR, 0,
                              glm::value_ptr(clearColor.value()));
  }
  return *this;
}
RenderContext &RenderContext::endRendering(GLuint framebuffer) {
  assert(m_renderingStarted && framebuffer != GL_NONE);

  glDeleteFramebuffers(1, &framebuffer);
  m_renderingStarted = false;
  return *this;
}

RenderContext &RenderContext::setGraphicsPipeline(const GraphicsPipeline &gp) {
  TracyGpuZone("SetGraphicsPipeline");

  {
    const auto &state = gp.m_depthStencilState;
    _setDepthTest(state.depthTest, state.depthCompareOp);
    _setDepthWrite(state.depthWrite);
  }

  {
    const auto &state = gp.m_rasterizerState;
    _setPolygonMode(state.polygonMode);
    _setCullMode(state.cullMode);
    _setPolygonOffset(state.polygonOffset);
    _setDepthClamp(state.depthClampEnable);
    _setScissorTest(state.scissorTest);
  }

  for (int32_t i{0}; i < gp.m_blendStates.size(); ++i)
    _setBlendState(i, gp.m_blendStates[i]);

  _setVertexArray(gp.m_vertexArray);
  _setShaderProgram(gp.m_program);

  return *this;
}

RenderContext &RenderContext::setUniform1f(const std::string_view name,
                                           float f) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniform1f(m_currentPipeline.m_program, location, f);
  return *this;
}
RenderContext &RenderContext::setUniform1i(const std::string_view name,
                                           int32_t i) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniform1i(m_currentPipeline.m_program, location, i);
  return *this;
}
RenderContext &RenderContext::setUniform1ui(const std::string_view name,
                                            uint32_t i) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniform1ui(m_currentPipeline.m_program, location, i);
  return *this;
}

RenderContext &RenderContext::setUniformVec3(const std::string_view name,
                                             const glm::vec3 &v) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniform3fv(m_currentPipeline.m_program, location, 1,
                      glm::value_ptr(v));
  return *this;
}
RenderContext &RenderContext::setUniformVec4(const std::string_view name,
                                             const glm::vec4 &v) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniform4fv(m_currentPipeline.m_program, location, 1,
                      glm::value_ptr(v));
  return *this;
}

RenderContext &RenderContext::setUniformMat3(const std::string_view name,
                                             const glm::mat3 &m) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniformMatrix3fv(m_currentPipeline.m_program, location, 1, GL_FALSE,
                            glm::value_ptr(m));
  return *this;
}
RenderContext &RenderContext::setUniformMat4(const std::string_view name,
                                             const glm::mat4 &m) {
  const auto location =
    glGetUniformLocation(m_currentPipeline.m_program, name.data());
  assert(location != GL_INVALID_INDEX);
  glProgramUniformMatrix4fv(m_currentPipeline.m_program, location, 1, GL_FALSE,
                            glm::value_ptr(m));
  return *this;
}

RenderContext &RenderContext::setViewport(const Rect2D &rect) {
  auto &current = m_currentPipeline.m_viewport;
  if (rect != current) {
    glViewport(rect.offset.x, rect.offset.y, rect.extent.width,
               rect.extent.height);
    current = rect;
  }
  return *this;
}
RenderContext &RenderContext::setScissor(const Rect2D &rect) {
  auto &current = m_currentPipeline.m_scissor;
  if (rect != current) {
    glScissor(rect.offset.x, rect.offset.y, rect.extent.width,
              rect.extent.height);
    current = rect;
  }
  return *this;
}

RenderContext &RenderContext::bindImage(GLuint unit, const Texture &texture,
                                        GLint mipLevel, GLenum access) {
  assert(texture && mipLevel < texture.m_numMipLevels);
  glBindImageTexture(unit, texture, mipLevel, GL_FALSE, 0, access,
                     static_cast<GLenum>(texture.m_pixelFormat));
  return *this;
}
RenderContext &RenderContext::bindTexture(GLuint unit, const Texture &texture,
                                          std::optional<GLuint> samplerId) {
  assert(texture);
  glBindTextureUnit(unit, texture);
  if (samplerId.has_value()) glBindSampler(unit, *samplerId);
  return *this;
}
RenderContext &RenderContext::bindUniformBuffer(GLuint index,
                                                const UniformBuffer &buffer) {
  assert(buffer);
  glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
  return *this;
}
RenderContext &RenderContext::bindStorageBuffer(GLuint index,
                                                const StorageBuffer &buffer) {
  assert(buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer);
  return *this;
}

RenderContext &RenderContext::drawFullScreenTriangle() {
  return draw({}, {},
              {
                .topology = PrimitiveTopology::TriangleList,
                .numVertices = 3,
              });
}
RenderContext &RenderContext::drawCube() {
  return draw({}, {},
              {
                .topology = PrimitiveTopology::TriangleList,
                .numVertices = 36,
              });
}

RenderContext &
RenderContext::draw(OptionalReference<const VertexBuffer> vertexBuffer,
                    OptionalReference<const IndexBuffer> indexBuffer,
                    const GeometryInfo &gi, uint32_t numInstances) {
  if (vertexBuffer.has_value()) _setVertexBuffer(*vertexBuffer);

  if (gi.numIndices > 0) {
    assert(indexBuffer.has_value());
    _setIndexBuffer(*indexBuffer);

    const auto stride = static_cast<GLsizei>(indexBuffer->get().getIndexType());
    const auto indices = reinterpret_cast<const void *>(
      static_cast<uint64_t>(stride) * gi.indexOffset);

    glDrawElementsInstancedBaseVertex(static_cast<GLenum>(gi.topology),
                                      gi.numIndices, getIndexDataType(stride),
                                      indices, numInstances, gi.vertexOffset);
  } else {
    glDrawArraysInstanced(static_cast<GLenum>(gi.topology), gi.vertexOffset,
                          gi.numVertices, numInstances);
  }
  return *this;
}

void RenderContext::_setupDebugCallback() {
#ifdef _DEBUG
  glDebugMessageCallback(_debugCallback, nullptr);
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

#  if 0
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                          GL_TRUE);
#  else
  glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0,
                        nullptr, GL_FALSE);
  glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0,
                        nullptr, GL_FALSE);
  glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0,
                        nullptr, GL_FALSE);
#  endif
#endif
}
void RenderContext::_debugCallback(GLenum source, GLenum type, GLuint id,
                                   GLenum severity, GLsizei length,
                                   const GLchar *message, const void *) {
  if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP)
    return;

  static std::unordered_map<GLenum, const char *> sources{
    {GL_DEBUG_SOURCE_API, "API"},
    {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "WINDOW SYSTEM"},
    {GL_DEBUG_SOURCE_SHADER_COMPILER, "SHADER COMPILER"},
    {GL_DEBUG_SOURCE_THIRD_PARTY, "THIRD PARTY"},
    {GL_DEBUG_SOURCE_APPLICATION, "APPLICATION"},
    {GL_DEBUG_SOURCE_OTHER, "OTHER"}};
  static std::unordered_map<GLenum, const char *> types{
    {GL_DEBUG_TYPE_ERROR, "ERROR"},
    {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DEPRECATED BEHAVIOR"},
    {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UNDEFINED BEHAVIOR"},
    {GL_DEBUG_TYPE_PORTABILITY, "PORTABILITY"},
    {GL_DEBUG_TYPE_PERFORMANCE, "PERFORMANCE"},
    {GL_DEBUG_TYPE_MARKER, "MARKER"},
    {GL_DEBUG_TYPE_OTHER, "OTHER"}};

  constexpr auto kMessageFormat = "{}: {}, raised from {}: {}";
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    SPDLOG_CRITICAL(kMessageFormat, id, types[type], sources[source], message);
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    SPDLOG_ERROR(kMessageFormat, id, types[type], sources[source], message);
    break;
  case GL_DEBUG_SEVERITY_LOW:
    SPDLOG_WARN(kMessageFormat, id, types[type], sources[source], message);
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    SPDLOG_INFO(kMessageFormat, id, types[type], sources[source], message);
    break;

  default:
    break;
  }
}

GLuint RenderContext::_createVertexArray(const VertexAttributes &attributes) {
  GLuint vao;
  glCreateVertexArrays(1, &vao);

  for (const auto &[location, attribute] : attributes) {
    const auto [type, size, normalized] = statAttribute(attribute.type);
    assert(type != GL_INVALID_INDEX);

    glEnableVertexArrayAttrib(vao, location);
    if (attribute.type == VertexAttribute::Type::Int4) {
      glVertexArrayAttribIFormat(vao, location, size, type, attribute.offset);
    } else {
      glVertexArrayAttribFormat(vao, location, size, type, normalized,
                                attribute.offset);
    }
    glVertexArrayAttribBinding(vao, location, 0);
  }
  return vao;
}

Texture RenderContext::_createImmutableTexture(Extent2D extent, uint32_t depth,
                                               PixelFormat pixelFormat,
                                               uint32_t numFaces,
                                               uint32_t numMipLevels,
                                               uint32_t numLayers) {
  assert(numMipLevels > 0);

  // http://github.khronos.org/KTX-Specification/#_texture_type

  GLenum target = numFaces == 6       ? GL_TEXTURE_CUBE_MAP
                  : depth > 0         ? GL_TEXTURE_3D
                  : extent.height > 0 ? GL_TEXTURE_2D
                                      : GL_TEXTURE_1D;
  assert(target == GL_TEXTURE_CUBE_MAP ? extent.width == extent.height : true);

  if (numLayers > 0) {
    switch (target) {
    case GL_TEXTURE_1D:
      target = GL_TEXTURE_1D_ARRAY;
      break;
    case GL_TEXTURE_2D:
      target = GL_TEXTURE_2D_ARRAY;
      break;
    case GL_TEXTURE_CUBE_MAP:
      target = GL_TEXTURE_CUBE_MAP_ARRAY;
      break;

    default:
      assert(false);
    }
  }

  GLuint id{GL_NONE};
  glCreateTextures(target, 1, &id);

  const auto internalFormat = static_cast<GLenum>(pixelFormat);
  switch (target) {
  case GL_TEXTURE_1D:
    glTextureStorage1D(id, numMipLevels, internalFormat, extent.width);
    break;
  case GL_TEXTURE_1D_ARRAY:
    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width,
                       numLayers);
    break;

  case GL_TEXTURE_2D:
    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width,
                       extent.height);
    break;
  case GL_TEXTURE_2D_ARRAY:
    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width,
                       extent.height, numLayers);
    break;

  case GL_TEXTURE_3D:
    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width,
                       extent.height, depth);
    break;

  case GL_TEXTURE_CUBE_MAP:
    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width,
                       extent.height);
    break;
  case GL_TEXTURE_CUBE_MAP_ARRAY:
    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width,
                       extent.height, numLayers * 6);
    break;
  }

  return Texture{id, target, pixelFormat, extent, numMipLevels, numLayers};
}

void RenderContext::_createFaceView(Texture &cubeMap, GLuint mipLevel,
                                    GLuint layer, GLuint face) {
  assert(cubeMap.m_type == GL_TEXTURE_CUBE_MAP or
         cubeMap.m_type == GL_TEXTURE_CUBE_MAP_ARRAY);

  if (cubeMap.m_view != GL_NONE) glDeleteTextures(1, &cubeMap.m_view);
  glGenTextures(1, &cubeMap.m_view);

  glTextureView(cubeMap.m_view, GL_TEXTURE_2D, cubeMap,
                static_cast<GLenum>(cubeMap.m_pixelFormat), mipLevel, 1,
                (layer * 6) + face, 1);
}

void RenderContext::_attachTexture(GLuint framebuffer, GLenum attachment,
                                   const AttachmentInfo &info) {
  const auto &[image, mipLevel, maybeLayer, maybeFace, _] = info;

  switch (image.m_type) {
  case GL_TEXTURE_CUBE_MAP:
  case GL_TEXTURE_CUBE_MAP_ARRAY:
    _createFaceView(image, mipLevel, maybeLayer.value_or(0),
                    maybeFace.value_or(0));
    glNamedFramebufferTexture(framebuffer, attachment, image.m_view, 0);
    break;

  case GL_TEXTURE_2D:
    glNamedFramebufferTexture(framebuffer, attachment, image, mipLevel);
    break;

  case GL_TEXTURE_2D_ARRAY:
    assert(maybeLayer.has_value());
    glNamedFramebufferTextureLayer(framebuffer, attachment, image, mipLevel,
                                   maybeLayer.value_or(0));
    break;

  default:
    assert(false);
  }
}

GLuint RenderContext::_createShader(GLenum type, const std::string_view code) {
  auto id = glCreateShader(type);
  const GLchar *strings{code.data()};
  glShaderSource(id, 1, &strings, nullptr);
  glCompileShader(id);

  GLint status;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  if (GL_FALSE == status) {
    GLint infoLogLength;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
    assert(infoLogLength > 0);
    std::string infoLog("", infoLogLength);
    glGetShaderInfoLog(id, infoLogLength, nullptr, infoLog.data());
    throw std::runtime_error{infoLog};
  };
  return id;
}
GLuint
RenderContext::_createShaderProgram(std::initializer_list<GLuint> shaders) {
  const auto program = glCreateProgram();

  for (auto shader : shaders)
    glAttachShader(program, shader);

  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (GL_FALSE == status) {
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    assert(infoLogLength > 0);
    std::string infoLog("", infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
    throw std::runtime_error{infoLog};
  }
  for (auto shader : shaders) {
    glDetachShader(program, shader);
    glDeleteShader(shader);
  }

  return program;
}

void RenderContext::_setShaderProgram(GLuint program) {
  assert(program != GL_NONE);
  if (auto &current = m_currentPipeline.m_program; current != program) {
    glUseProgram(program);
    current = program;
  }
}
void RenderContext::_setVertexArray(GLuint vao) {
  if (GL_NONE == vao) vao = m_dummyVAO;

  if (auto &current = m_currentPipeline.m_vertexArray; vao != current) {
    glBindVertexArray(vao);
    current = vao;
  }
}
void RenderContext::_setVertexBuffer(const VertexBuffer &vertexBuffer) {
  const auto vao = m_currentPipeline.m_vertexArray;
  assert(vertexBuffer && vao != GL_NONE);
  glVertexArrayVertexBuffer(vao, 0, vertexBuffer.m_id, 0,
                            vertexBuffer.getStride());
}
void RenderContext::_setIndexBuffer(const IndexBuffer &indexBuffer) {
  const auto vao = m_currentPipeline.m_vertexArray;
  assert(indexBuffer && vao != GL_NONE);
  glVertexArrayElementBuffer(vao, indexBuffer.m_id);
}

void RenderContext::_setDepthTest(bool enabled, CompareOp depthFunc) {
  auto &current = m_currentPipeline.m_depthStencilState;
  if (enabled != current.depthTest) {
    enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    current.depthTest = enabled;
  }
  if (enabled && depthFunc != current.depthCompareOp) {
    glDepthFunc(static_cast<GLenum>(depthFunc));
    current.depthCompareOp = depthFunc;
  }
}
void RenderContext::_setDepthWrite(bool enabled) {
  auto &current = m_currentPipeline.m_depthStencilState;
  if (enabled != current.depthWrite) {
    glDepthMask(enabled);
    current.depthWrite = enabled;
  }
}

void RenderContext::_setPolygonMode(PolygonMode polygonMode) {
  auto &current = m_currentPipeline.m_rasterizerState.polygonMode;
  if (polygonMode != current) {
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode));
    current = polygonMode;
  }
}
void RenderContext::_setPolygonOffset(
  std::optional<PolygonOffset> polygonOffset) {
  auto &current = m_currentPipeline.m_rasterizerState;
  if (polygonOffset != current.polygonOffset) {
    const auto offsetCap = getPolygonOffsetCap(current.polygonMode);
    if (polygonOffset.has_value()) {
      glEnable(offsetCap);
      glPolygonOffset(polygonOffset->factor, polygonOffset->units);
    } else {
      glDisable(offsetCap);
    }
    current.polygonOffset = polygonOffset;
  }
}
void RenderContext::_setCullMode(CullMode cullMode) {
  auto &current = m_currentPipeline.m_rasterizerState.cullMode;
  if (cullMode != current) {
    if (cullMode != CullMode::None) {
      if (current == CullMode::None) glEnable(GL_CULL_FACE);
      glCullFace(static_cast<GLenum>(cullMode));
    } else {
      glDisable(GL_CULL_FACE);
    }
    current = cullMode;
  }
}
void RenderContext::_setDepthClamp(bool enabled) {
  auto &current = m_currentPipeline.m_rasterizerState.depthClampEnable;
  if (enabled != current) {
    enabled ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP);
    current = enabled;
  }
}
void RenderContext::_setScissorTest(bool enabled) {
  auto &current = m_currentPipeline.m_rasterizerState.scissorTest;
  if (enabled != current) {
    enabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    current = enabled;
  }
}

void RenderContext::_setBlendState(GLuint index, const BlendState &state) {
  auto &current = m_currentPipeline.m_blendStates[index];
  if (state != current) {
    if (state.enabled != current.enabled) {
      state.enabled ? glEnablei(GL_BLEND, index) : glDisablei(GL_BLEND, index);
      current.enabled = state.enabled;
    }
    if (state.enabled) {
      if (state.colorOp != current.colorOp or
          state.alphaOp != current.alphaOp) {
        glBlendEquationSeparatei(index, static_cast<GLenum>(state.colorOp),
                                 static_cast<GLenum>(state.alphaOp));

        current.colorOp = state.colorOp;
        current.alphaOp = state.alphaOp;
      }
      if (state.srcColor != current.srcColor or
          state.destColor != current.destColor or
          state.srcAlpha != current.srcAlpha or
          state.destAlpha != current.destAlpha) {
        glBlendFuncSeparatei(index, static_cast<GLenum>(state.srcColor),
                             static_cast<GLenum>(state.destColor),
                             static_cast<GLenum>(state.srcAlpha),
                             static_cast<GLenum>(state.destAlpha));

        current.srcColor = state.srcColor;
        current.destColor = state.destColor;
        current.srcAlpha = state.srcAlpha;
        current.destAlpha = state.destAlpha;
      }
    }
  }
}

//
// DebugMarker class:
//

DebugMarker::DebugMarker(const std::string_view name) {
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.data());
}
DebugMarker::~DebugMarker() { glPopDebugGroup(); }
