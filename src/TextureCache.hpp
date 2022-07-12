#pragma once

#include "RenderContext.hpp"
#include <unordered_map>
#include <filesystem>

class TextureCache {
public:
  TextureCache(RenderContext &);

  std::shared_ptr<Texture> load(std::filesystem::path);

private:
  RenderContext &m_renderContext;
  std::unordered_map<std::size_t, std::shared_ptr<Texture>> m_textures;
};
