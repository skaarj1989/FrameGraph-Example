#include "VertexFormat.hpp"
#include "Hash.hpp"
#include <cassert>
#include <algorithm>

int32_t getSize(VertexAttribute::Type type) {
  switch (type) {
    using enum VertexAttribute::Type;
  case Float:
    return sizeof(float);
  case Float2:
    return sizeof(float) * 2;
  case Float3:
    return sizeof(float) * 3;
  case Float4:
    return sizeof(float) * 4;

  case Int4:
    return sizeof(int32_t) * 4;

  case UByte4_Norm:
    return sizeof(uint8_t) * 4;
  }
  assert(false);
  return 0;
}

//
// VertexFormat class:
//

std::size_t VertexFormat::getHash() const { return m_hash; }

const VertexAttributes &VertexFormat::getAttributes() const {
  return m_attributes;
}
bool VertexFormat::contains(AttributeLocation location) const {
  return m_attributes.contains(static_cast<int32_t>(location));
}
bool VertexFormat::contains(
  std::initializer_list<AttributeLocation> locations) const {
  return std::all_of(
    std::cbegin(locations), std::cend(locations), [&](auto location) {
      return std::any_of(m_attributes.cbegin(), m_attributes.cend(),
                         [v = static_cast<int32_t>(location)](const auto &p) {
                           return p.first == v;
                         });
    });
}

uint32_t VertexFormat::getStride() const { return m_stride; }

VertexFormat::VertexFormat(std::size_t hash, VertexAttributes &&attributes,
                           uint32_t stride)
    : m_hash{hash}, m_attributes{attributes}, m_stride{stride} {}

//
// Builder:
//

using Builder = VertexFormat::Builder;

Builder &Builder::setAttribute(AttributeLocation location,
                               const VertexAttribute &attribute) {
  m_attributes.insert_or_assign(static_cast<int32_t>(location), attribute);
  return *this;
}

std::shared_ptr<VertexFormat> Builder::build() {
  uint32_t stride{0};
  std::size_t hash{0};
  for (const auto &[location, attribute] : m_attributes) {
    stride += getSize(attribute.type);
    hashCombine(hash, location, attribute);
  }

  if (const auto it = m_cache.find(hash); it != m_cache.cend())
    if (auto vertexFormat = it->second.lock(); vertexFormat)
      return vertexFormat;

  auto vertexFormat = std::make_shared<VertexFormat>(
    VertexFormat{hash, std::move(m_attributes), stride});
  m_cache.insert_or_assign(hash, vertexFormat);
  return vertexFormat;
}

//
// Utility:
//

std::vector<std::string> buildDefines(const VertexFormat &vertexFormat) {
  constexpr auto kMaxNumVertexDefines = 6;
  std::vector<std::string> defines;
  defines.reserve(kMaxNumVertexDefines);

  if (vertexFormat.contains(AttributeLocation::Color_0))
    defines.emplace_back("HAS_COLOR");
  if (vertexFormat.contains(AttributeLocation::Normal))
    defines.emplace_back("HAS_NORMAL");
  if (vertexFormat.contains(AttributeLocation::TexCoord_0)) {
    defines.emplace_back("HAS_TEXCOORD0");
    if (vertexFormat.contains(
          {AttributeLocation::Tangent, AttributeLocation::Bitangent})) {
      defines.emplace_back("HAS_TANGENTS");
    }
  }
  if (vertexFormat.contains(AttributeLocation::TexCoord_1))
    defines.emplace_back("HAS_TEXCOORD1");
  if (vertexFormat.contains(
        {AttributeLocation::Joints, AttributeLocation::Weights}))
    defines.emplace_back("IS_SKINNED");

  return defines;
}
