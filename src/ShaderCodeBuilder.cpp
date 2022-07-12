#include "ShaderCodeBuilder.hpp"
#include "FileUtility.hpp"
#include "spdlog/spdlog.h"

#include "Tracy.hpp"

#include <unordered_set>
#include <regex>

namespace {

static const std::filesystem::path kShadersPath{"../shaders"};

// https://stackoverflow.com/questions/26492513/write-c-regular-expression-to-match-a-include-preprocessing-directive
// https://stackoverflow.com/questions/42139302/using-regex-to-filter-for-preprocessor-directives

[[nodiscard]] auto findIncludes(const std::string &src) {
  // Source: #include "filename"
  // [1] "
  // [2] filename
  // [3] "

  // TODO: Ignore directives inside a comment
  static const std::regex kIncludePattern(
    R"(#\s*include\s*([<"])([^>"]+)([>"]))");
  return std::sregex_iterator{src.cbegin(), src.cend(), kIncludePattern};
}

[[nodiscard]] std::tuple<std::string, bool> statInclude(const std::smatch &m) {
  const auto isBetween = [&](auto &&a, auto &&b) {
    return m[1].compare(a) == 0 && m[3].compare(b) == 0;
  };

  if (isBetween("\"", "\""))
    return {m[2].str(), true};
  else if (isBetween("<", ">"))
    return {m[2].str(), false};

  return {"", false};
}

void resolveInclusions(std::string &src,
                       std::unordered_set<std::size_t> &alreadyIncluded,
                       int32_t level, const std::filesystem::path &currentDir) {
  ptrdiff_t offset{0};

  // Copy to avoid dereference of invalidated string iterator
  std::string temp{src};
  for (auto match = findIncludes(temp); match != std::sregex_iterator{};
       ++match) {
    const auto lineLength = match->_At(0).length();
    auto [filename, isRelative] = statInclude(*match);
    if (filename.empty()) {
      SPDLOG_WARN("Ill-formed directive: {}", match->_At(0).str());
      src.erase(match->position() + offset, lineLength);
      offset -= lineLength;
      continue;
    }
    const auto next =
      ((isRelative ? currentDir : kShadersPath) / filename).lexically_normal();
    if (!alreadyIncluded.insert(std::filesystem::hash_value(next)).second) {
      SPDLOG_TRACE("Already included: {}, skipping", filename);
      src.erase(match->position() + offset, lineLength);
      offset -= lineLength;
      continue;
    }

    auto codeChunk = readText(next);
    if (!codeChunk.empty()) {
      resolveInclusions(codeChunk, alreadyIncluded, level + 1,
                        next.parent_path());
    } else {
      codeChunk = std::format(R"(#error "{}" not found!)", filename);
    }

    src.replace(match->position() + offset, lineLength, codeChunk);
    offset += codeChunk.length() - lineLength;
  }
}

} // namespace

//
// ShaderCodeBuilder class:
//

ShaderCodeBuilder &
ShaderCodeBuilder::setDefines(const std::vector<std::string> &defines) {
  m_defines = defines;
  return *this;
}
ShaderCodeBuilder &ShaderCodeBuilder::addDefine(const std::string &s) {
  m_defines.emplace_back(s);
  return *this;
}

ShaderCodeBuilder &ShaderCodeBuilder::replace(const std::string &phrase,
                                              const std::string_view s) {
  m_patches.insert_or_assign(phrase, s);
  return *this;
}

std::string ShaderCodeBuilder::build(const std::filesystem::path &p) {
  const auto filePath = kShadersPath / p;
  auto sourceCode = readText(filePath);
  if (sourceCode.empty()) throw std::runtime_error{"Empty string"};

  ZoneScoped;

  std::unordered_set<std::size_t> alreadyIncluded;
  resolveInclusions(sourceCode, alreadyIncluded, 0, filePath.parent_path());

  for (const auto &[phrase, patch] : m_patches) {
    const auto pos = sourceCode.find(phrase);
    if (pos != std::string::npos)
      sourceCode.replace(pos, phrase.length(), patch);
  }

  auto versionDirectivePos = sourceCode.find("#version");
  assert(versionDirectivePos != std::string::npos);

  if (!m_defines.empty()) {
    std::ostringstream oss;
    std::transform(m_defines.cbegin(), m_defines.cend(),
                   std::ostream_iterator<std::string>{oss, "\n"},
                   [](const auto &s) { return std::format("#define {}", s); });

    const auto endOfVersionLine =
      sourceCode.find_first_of("\n", versionDirectivePos) + 1;
    sourceCode.insert(endOfVersionLine, oss.str());
  }
  return sourceCode;
}
