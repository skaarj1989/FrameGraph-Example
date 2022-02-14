#pragma once

#include "TypeDefs.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <optional>

enum class PixelFormat : GLenum {
  Unknown = GL_NONE,

  R8_UNorm = GL_R8,

  RGB8_UNorm = GL_RGB8,
  RGBA8_UNorm = GL_RGBA8,

  RGB8_SNorm = GL_RGB8_SNORM,
  RGBA8_SNorm = GL_RGBA8_SNORM,

  R16F = GL_R16F,
  RG16F = GL_RG16F,
  RGB16F = GL_RGB16F,
  RGBA16F = GL_RGBA16F,

  RGB32F = GL_RGB32F,

  RGBA32F = GL_RGBA32F,

  RGBA32UI = GL_RGBA32UI,

  Depth16 = GL_DEPTH_COMPONENT16,
  Depth24 = GL_DEPTH_COMPONENT24,
  Depth32F = GL_DEPTH_COMPONENT32F
};

enum class TextureType : GLenum {
  Texture2D = GL_TEXTURE_2D,
  CubeMap = GL_TEXTURE_CUBE_MAP
};

class Texture {
  friend class RenderContext;

public:
  Texture() = default;
  Texture(const Texture &) = delete;
  Texture(Texture &&) noexcept;
  ~Texture();

  Texture &operator=(const Texture &) = delete;
  Texture &operator=(Texture &&) noexcept;

  explicit operator bool() const;

  [[nodiscard]] GLenum getType() const;

  [[nodiscard]] Extent2D getExtent() const;
  [[nodiscard]] uint32_t getDepth() const;
  [[nodiscard]] uint32_t getNumMipLevels() const;
  [[nodiscard]] uint32_t getNumLayers() const;
  [[nodiscard]] PixelFormat getPixelFormat() const;

private:
  Texture(GLuint id, GLenum type, PixelFormat, Extent2D, uint32_t numMipLevels,
          uint32_t numLayers);

  operator GLuint() const;

private:
  GLuint m_id{GL_NONE};
  GLenum m_type{GL_NONE};

  GLuint m_view{GL_NONE};

  Extent2D m_extent{0u};
  uint32_t m_depth{0u};
  uint32_t m_numMipLevels{1u};
  uint32_t m_numLayers{0u};

  PixelFormat m_pixelFormat{PixelFormat::Unknown};
};

enum class TexelFilter : GLenum { Nearest = GL_NEAREST, Linear = GL_LINEAR };
enum class MipmapMode : GLenum {
  None = GL_NONE,
  Nearest = GL_NEAREST,
  Linear = GL_LINEAR
};

enum class SamplerAddressMode : GLenum {
  Repeat = GL_REPEAT,
  MirroredRepeat = GL_MIRRORED_REPEAT,
  ClampToEdge = GL_CLAMP_TO_EDGE,
  ClampToBorder = GL_CLAMP_TO_BORDER,
  MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
};

struct SamplerInfo {
  TexelFilter minFilter{TexelFilter::Nearest};
  MipmapMode mipmapMode{MipmapMode::Linear};
  TexelFilter magFilter{TexelFilter::Linear};

  SamplerAddressMode addressModeS{SamplerAddressMode::Repeat};
  SamplerAddressMode addressModeT{SamplerAddressMode::Repeat};
  SamplerAddressMode addressModeR{SamplerAddressMode::Repeat};

  std::optional<CompareOp> compareOp{};
  glm::vec4 borderColor{0.0f};
};

[[nodiscard]] uint32_t calcMipLevels(uint32_t size);
[[nodiscard]] glm::uvec3 calcMipSize(const glm::uvec3 &baseSize,
                                     uint32_t level);

[[nodiscard]] const char *toString(PixelFormat);
