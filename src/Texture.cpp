#include "Texture.hpp"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

//
// Texture class:
//

Texture::Texture(Texture &&other) noexcept
    : m_id{other.m_id}, m_type{other.m_type}, m_view{other.m_view},
      m_extent{other.m_extent}, m_depth{other.m_depth},
      m_numMipLevels{other.m_numMipLevels}, m_numLayers{other.m_numLayers},
      m_pixelFormat{other.m_pixelFormat} {
  memset(&other, 0, sizeof(Texture));
}
Texture::~Texture() {
  if (m_id != GL_NONE) SPDLOG_WARN("Texture leak: {}", m_id);
}

Texture &Texture::operator=(Texture &&rhs) noexcept {
  if (this != &rhs) {
    memcpy(this, &rhs, sizeof(Texture));
    memset(&rhs, 0, sizeof(Texture));
  }
  return *this;
}

Texture::operator bool() const { return m_id != GL_NONE; }

GLenum Texture::getType() const { return m_type; }

Extent2D Texture::getExtent() const { return m_extent; }
uint32_t Texture::getDepth() const { return m_depth; }
uint32_t Texture::getNumMipLevels() const { return m_numMipLevels; }
uint32_t Texture::getNumLayers() const { return m_numLayers; }
PixelFormat Texture::getPixelFormat() const { return m_pixelFormat; }

Texture::Texture(GLuint id, GLenum type, PixelFormat pixelFormat,
                 Extent2D extent, uint32_t depth, uint32_t numMipLevels,
                 uint32_t numLayers)
    : m_id{id}, m_type{type}, m_extent{extent}, m_depth{depth},
      m_numMipLevels{numMipLevels}, m_numLayers{numLayers}, m_pixelFormat{
                                                              pixelFormat} {}

Texture::operator GLuint() const { return m_id; }

//
// Utility:
//

uint32_t calcMipLevels(uint32_t size) {
  return glm::floor(glm::log2(static_cast<float>(size))) + 1u;
}
glm::uvec3 calcMipSize(const glm::uvec3 &baseSize, uint32_t level) {
  return glm::vec3{baseSize} * glm::pow(0.5f, static_cast<float>(level));
}

// clang-format off
const char *toString(PixelFormat pixelFormat) {
  switch (pixelFormat) {
    using enum PixelFormat;

  case R8_UNorm: return "R8_Unorm";

  case RGB8_UNorm: return "RGB8_UNorm";
  case RGBA8_UNorm: return "RGBA8_UNorm";
  
  case RGB8_SNorm: return "RGB8_SNorm";
  case RGBA8_SNorm: return "RGBA8_SNorm";

  case R16F: return"R16F";
  case RG16F: return "RG16F";
  case RGB16F: return "RGB16F";
  case RGBA16F: return "RGBA16F";

  case RGB32F: return "RGB32F";

  case RGBA32UI: return "RGBA32UI";

  case Depth16: return "Depth16";
  case Depth24: return "Depth24";
  case Depth32F: return "Depth32F";
  }

  return "Undefined";
}
// clang-format on
