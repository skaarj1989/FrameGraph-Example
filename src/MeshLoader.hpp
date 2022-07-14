#pragma once

#include "Mesh.hpp"
#include "TextureCache.hpp"

std::shared_ptr<Mesh> loadMesh(const std::filesystem::path &, RenderContext &,
                               TextureCache &);
