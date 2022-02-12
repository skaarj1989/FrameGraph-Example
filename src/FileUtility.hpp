#pragma once

#include <filesystem>

[[nodiscard]] std::filesystem::path
adjustPath(const std::filesystem::path &, const std::filesystem::path &root);

[[nodiscard]] std::string readText(const std::filesystem::path &);
