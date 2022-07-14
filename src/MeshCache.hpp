#pragma once

#include "MaterialCache.hpp"
#include "Mesh.hpp"

class MeshCache {
public:
  MeshCache(RenderContext &, TextureCache &);

  std::shared_ptr<Mesh> load(std::filesystem::path);

private:
  RenderContext &m_renderContext;
  TextureCache &m_textureCache;
  std::unordered_map<std::size_t, std::shared_ptr<Mesh>> m_meshes;
};
