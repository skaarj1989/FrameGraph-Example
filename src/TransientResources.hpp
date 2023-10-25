#pragma once

#include "FrameGraphTexture.hpp"
#include "FrameGraphBuffer.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

class RenderContext;

class TransientResources {
public:
  TransientResources() = delete;
  explicit TransientResources(RenderContext &);
  TransientResources(const TransientResources &) = delete;
  TransientResources(TransientResources &&) noexcept = delete;
  ~TransientResources();

  TransientResources &operator=(const TransientResources &) = delete;
  TransientResources &operator=(TransientResources &&) noexcept = delete;

  void update(float dt);

  [[nodiscard]] Texture *acquireTexture(const FrameGraphTexture::Desc &);
  void releaseTexture(const FrameGraphTexture::Desc &, Texture *);

  [[nodiscard]] Buffer *acquireBuffer(const FrameGraphBuffer::Desc &);
  void releaseBuffer(const FrameGraphBuffer::Desc &, Buffer *);

private:
  RenderContext &m_renderContext;

  std::vector<std::unique_ptr<Texture>> m_textures;
  std::vector<std::unique_ptr<Buffer>> m_buffers;

  template <typename T> struct ResourceEntry {
    T resource;
    float life;
  };
  template <typename T> using ResourcePool = std::vector<ResourceEntry<T>>;

  std::unordered_map<std::size_t, ResourcePool<Texture *>> m_texturePools;
  std::unordered_map<std::size_t, ResourcePool<Buffer *>> m_bufferPools;
};
