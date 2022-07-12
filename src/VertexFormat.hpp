#pragma once

#include "VertexAttributes.hpp"
#include <memory>
#include <unordered_map>

// NOTE: Make sure that the following locations match with the
// shaders/Geometry.vert
enum class AttributeLocation : int32_t {
  Position = 0,
  Color_0,
  Normal,
  TexCoord_0,
  TexCoord_1,
  Tangent,
  Bitangent,

  Joints,
  Weights,
};

class VertexFormat {
public:
  VertexFormat() = delete;
  VertexFormat(const VertexFormat &) = delete;
  VertexFormat(VertexFormat &&) noexcept = default;
  ~VertexFormat() = default;

  VertexFormat &operator=(const VertexFormat &) = delete;
  VertexFormat &operator=(VertexFormat &&) noexcept = default;

  [[nodiscard]] std::size_t getHash() const;

  [[nodiscard]] const VertexAttributes &getAttributes() const;
  [[nodiscard]] bool contains(AttributeLocation) const;
  [[nodiscard]] bool contains(std::initializer_list<AttributeLocation>) const;

  [[nodiscard]] uint32_t getStride() const;

  class Builder final {
  public:
    Builder() = default;
    Builder(const Builder &) = delete;
    Builder(Builder &&) noexcept = delete;
    ~Builder() = default;

    Builder &operator=(const Builder &) = delete;
    Builder &operator=(Builder &&) noexcept = delete;

    Builder &setAttribute(AttributeLocation, const VertexAttribute &);

    [[nodiscard]] std::shared_ptr<VertexFormat> build();

  private:
    VertexAttributes m_attributes;

    using Cache = std::unordered_map<std::size_t, std::weak_ptr<VertexFormat>>;
    inline static Cache m_cache;
  };

private:
  VertexFormat(std::size_t hash, VertexAttributes &&, uint32_t stride);

private:
  const std::size_t m_hash{0u};
  const VertexAttributes m_attributes;
  const uint32_t m_stride{0};
};

[[nodiscard]] int32_t getSize(VertexAttribute::Type);

[[nodiscard]] std::vector<std::string> buildDefines(const VertexFormat &);
