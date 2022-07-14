#include "TextureCache.hpp"
#include "TextureLoader.hpp"

TextureCache::TextureCache(RenderContext &rc) : m_renderContext{rc} {}

std::shared_ptr<Texture> TextureCache::load(std::filesystem::path p) {
  p = std::filesystem::absolute(p);
  const auto h = std::filesystem::hash_value(p);
  if (auto it = m_textures.find(h); it != m_textures.cend()) {
    return it->second;
  }

  auto texture = loadTexture(p, m_renderContext);
  m_textures[h] = texture;
  return texture;
}
