#include "TransientResources.hpp"
#include "RenderContext.hpp"
#include "Hash.hpp"
#include "spdlog/spdlog.h"

#include "Tracy.hpp"

namespace std {

template <> struct hash<FrameGraphTexture::Desc> {
  std::size_t operator()(const FrameGraphTexture::Desc &desc) const noexcept {
    std::size_t h{0};
    hashCombine(h, desc.extent.width, desc.extent.height, desc.depth,
                desc.numMipLevels, desc.layers, desc.format, desc.shadowSampler,
                desc.wrapMode, desc.filter);
    return h;
  }
};
template <> struct hash<FrameGraphBuffer::Desc> {
  std::size_t operator()(const FrameGraphBuffer::Desc &desc) const noexcept {
    std::size_t h{0};
    hashCombine(h, desc.size);
    return h;
  }
};

} // namespace std

namespace {

void heartbeat(auto &objects, auto &pools, float dt, auto &&deleter) {
  constexpr auto kMaxIdleTime = 1.0f; // in seconds

  auto poolIt = pools.begin();
  while (poolIt != pools.end()) {
    auto &[_, pool] = *poolIt;
    if (pool.empty()) {
      poolIt = pools.erase(poolIt);
    } else {
      auto objectIt = pool.begin();
      while (objectIt != pool.cend()) {
        auto &[object, idleTime] = *objectIt;
        idleTime += dt;
        if (idleTime >= kMaxIdleTime) {
          deleter(*object);
          SPDLOG_INFO("Released resource: {}", fmt::ptr(object));
          objectIt = pool.erase(objectIt);
        } else {
          ++objectIt;
        }
      }
      ++poolIt;
    }
  }
  objects.erase(std::remove_if(objects.begin(), objects.end(),
                               [](auto &object) { return !(*object); }),
                objects.end());
}

} // namespace

//
// TransientResources class:
//

TransientResources::TransientResources(RenderContext &rc)
    : m_renderContext{rc} {}
TransientResources::~TransientResources() {
  for (auto &texture : m_textures)
    m_renderContext.destroy(*texture);
  for (auto &buffer : m_buffers)
    m_renderContext.destroy(*buffer);
}

void TransientResources::update(float dt) {
  ZoneScoped;

  const auto deleter = [&](auto &object) { m_renderContext.destroy(object); };
  heartbeat(m_textures, m_texturePools, dt, deleter);
  heartbeat(m_buffers, m_bufferPools, dt, deleter);
}

Texture *
TransientResources::acquireTexture(const FrameGraphTexture::Desc &desc) {
  const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
  auto &pool = m_texturePools[h];
  if (pool.empty()) {
    Texture texture;
    if (desc.depth > 0) {
      texture =
        m_renderContext.createTexture3D(desc.extent, desc.depth, desc.format);
    } else {
      texture = m_renderContext.createTexture2D(desc.extent, desc.format,
                                                desc.numMipLevels, desc.layers);
    }

    glm::vec4 borderColor{0.0f};
    auto addressMode = SamplerAddressMode::ClampToEdge;
    switch (desc.wrapMode) {
    case WrapMode::ClampToEdge:
      addressMode = SamplerAddressMode::ClampToEdge;
      break;
    case WrapMode::ClampToOpaqueBlack:
      addressMode = SamplerAddressMode::ClampToBorder;
      borderColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
      break;
    case WrapMode::ClampToOpaqueWhite:
      addressMode = SamplerAddressMode::ClampToBorder;
      borderColor = glm::vec4{1.0f};
      break;
    }
    SamplerInfo samplerInfo{
      .minFilter = desc.filter,
      .mipmapMode =
        desc.numMipLevels > 1 ? MipmapMode::Nearest : MipmapMode::None,
      .magFilter = desc.filter,
      .addressModeS = addressMode,
      .addressModeT = addressMode,
      .addressModeR = addressMode,
      .borderColor = borderColor,
    };
    if (desc.shadowSampler) samplerInfo.compareOp = CompareOp::LessOrEqual;
    m_renderContext.setupSampler(texture, samplerInfo);

    m_textures.push_back(std::make_unique<Texture>(std::move(texture)));
    auto *ptr = m_textures.back().get();
    SPDLOG_INFO("Created texture: {}", fmt::ptr(ptr));
    return ptr;
  } else {
    auto *texture = pool.back().resource;
    pool.pop_back();
    return texture;
  }
}
void TransientResources::releaseTexture(const FrameGraphTexture::Desc &desc,
                                        Texture *texture) {
  const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
  m_texturePools[h].push_back({texture, 0.0f});
}

Buffer *TransientResources::acquireBuffer(const FrameGraphBuffer::Desc &desc) {
  const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
  auto &pool = m_bufferPools[h];
  if (pool.empty()) {
    auto buffer = m_renderContext.createBuffer(desc.size);
    m_buffers.push_back(std::make_unique<Buffer>(std::move(buffer)));
    auto *ptr = m_buffers.back().get();
    SPDLOG_INFO("Created buffer: {}", fmt::ptr(ptr));
    return ptr;
  } else {
    auto *buffer = pool.back().resource;
    pool.pop_back();
    return buffer;
  }
}
void TransientResources::releaseBuffer(const FrameGraphBuffer::Desc &desc,
                                       Buffer *buffer) {
  const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
  m_bufferPools[h].push_back({std::move(buffer), 0.0f});
}
