#pragma once

#include "Material.hpp"
#include <filesystem>

class RenderContext;

[[nodiscard]] std::shared_ptr<Material>
loadMaterial(const std::filesystem::path &, RenderContext &);
