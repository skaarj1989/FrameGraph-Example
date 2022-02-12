#pragma once

#include "Texture.hpp"
#include <filesystem>

class RenderContext;

[[nodiscard]] std::shared_ptr<Texture>
loadTexture(const std::filesystem::path &, RenderContext &);
