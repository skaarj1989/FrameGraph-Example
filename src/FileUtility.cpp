#include "FileUtility.hpp"
#include <fstream>

std::filesystem::path adjustPath(const std::filesystem::path &p,
                                 const std::filesystem::path &root) {
  return (p.is_absolute() ? p : root.parent_path() / p).lexically_normal();
}

std::string readText(const std::filesystem::path &p) {
  std::ifstream file(p.string().c_str());
  if (!file.is_open())
    throw std::runtime_error{"Failed to open file: " + p.string()};

  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}
