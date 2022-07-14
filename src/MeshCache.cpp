#include "MeshCache.hpp"
#include "MeshLoader.hpp"

MeshCache::MeshCache(RenderContext &rc, TextureCache &textureCache)
    : m_renderContext{rc}, m_textureCache{textureCache} {}

std::shared_ptr<Mesh> MeshCache::load(std::filesystem::path p) {
  p = std::filesystem::absolute(p);
  const auto h = std::filesystem::hash_value(p);
  if (auto it = m_meshes.find(h); it != m_meshes.cend()) {
    return it->second;
  }

  auto mesh = loadMesh(p, m_renderContext, m_textureCache);
  m_meshes[h] = mesh;
  return mesh;
}
