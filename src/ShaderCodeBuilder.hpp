#pragma once

#include <filesystem>
#include <unordered_map>

class ShaderCodeBuilder {
public:
  ShaderCodeBuilder() = default;
  ShaderCodeBuilder(const ShaderCodeBuilder &) = delete;
  ShaderCodeBuilder(ShaderCodeBuilder &&) noexcept = delete;
  ~ShaderCodeBuilder() = default;

  ShaderCodeBuilder &operator=(const ShaderCodeBuilder &) = delete;
  ShaderCodeBuilder &operator=(ShaderCodeBuilder &&) noexcept = delete;

  ShaderCodeBuilder &setDefines(const std::vector<std::string> &);
  ShaderCodeBuilder &addDefine(const std::string &);
  template <typename T> ShaderCodeBuilder &addDefine(const std::string &, T &&);

  ShaderCodeBuilder &replace(const std::string &phrase,
                             const std::string_view s);

  [[nodiscard]] std::string build(const std::filesystem::path &p);

private:
  std::vector<std::string> m_defines;
  std::unordered_map<std::string, std::string> m_patches;
};

#include "ShaderCodeBuilder.inl"
