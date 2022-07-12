#pragma once

#include "TextureCache.hpp"
#include "Material.hpp"

class MaterialCache {
public:
  MaterialCache(TextureCache &);

  std::shared_ptr<Material> load(std::filesystem::path);

private:
  TextureCache &m_textureCache;
  std::unordered_map<std::size_t, std::shared_ptr<Material>> m_materials;
};
