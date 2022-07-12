#pragma once

#include "Material.hpp"
#include "TextureCache.hpp"

[[nodiscard]] std::shared_ptr<Material>
loadMaterial(const std::filesystem::path &, TextureCache &);
