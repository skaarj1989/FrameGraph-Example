#include "MaterialCache.hpp"
#include "MaterialLoader.hpp"

MaterialCache::MaterialCache(TextureCache &textureCache)
    : m_textureCache{textureCache} {}

std::shared_ptr<Material> MaterialCache::load(std::filesystem::path p) {
  p = std::filesystem::absolute(p);
  const auto h = std::filesystem::hash_value(p);
  if (auto it = m_materials.find(h); it != m_materials.cend()) {
    return it->second;
  }

  auto material = loadMaterial(p, m_textureCache);
  m_materials[h] = material;
  return material;
}
