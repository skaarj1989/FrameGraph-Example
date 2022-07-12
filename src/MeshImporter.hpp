#pragma once

#include "Mesh.hpp"

#include "assimp/BaseImporter.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <vector>
#include <map>
#include <set>
#include <tuple>

using AttributesMap = std::map<AttributeLocation, VertexAttribute>;

class VertexInfo {
public:
  VertexInfo() = default;
  VertexInfo(const VertexInfo &) = delete;
  VertexInfo(VertexInfo &&) noexcept = default;
  ~VertexInfo() = default;

  VertexInfo &operator=(const VertexInfo &) = delete;
  VertexInfo &operator=(VertexInfo &&) = default;

  [[nodiscard]] uint32_t getStride() const;
  [[nodiscard]] const VertexAttribute &getAttribute(AttributeLocation) const;
  [[nodiscard]] const AttributesMap &getAttributes() const;

  class Builder {
  public:
    explicit Builder() = default;
    Builder(const Builder &) = delete;
    Builder(Builder &&) noexcept = delete;
    ~Builder() = default;

    Builder &operator=(const Builder &) = delete;
    Builder &operator=(Builder &&) = delete;

    Builder &add(AttributeLocation, VertexAttribute::Type);

    [[nodiscard]] VertexInfo build();

  private:
    int32_t m_currentOffset{0};
    AttributesMap m_attributes;
  };

private:
  VertexInfo(int32_t stride, AttributesMap &&);

private:
  int32_t m_stride{0};
  AttributesMap m_attributes;
};

struct TextureInfo {
  std::string name;
  std::string path;
  uint32_t uvIndex{0};

  auto operator<=>(const TextureInfo &) const = default;
};

struct MaterialInfo {
  std::string name;
  BlendMode blendMode;
  std::set<TextureInfo> textures;
  std::string fragCode;
};

struct SubMeshInfo {
  std::string name;
  GeometryInfo geometryInfo;
  MaterialInfo materialInfo;
  AABB aabb;
};

using ByteBuffer = std::vector<std::byte>;

class MeshImporter {
public:
  MeshImporter(const aiScene *);

  void _findVertexFormat();

  void _processMeshes();
  [[nodiscard]] SubMeshInfo _processMesh(const aiMesh &);
  [[nodiscard]] MaterialInfo _processMaterial(uint32_t id, const aiMaterial &);

  void _fillVertexBuffer(const aiMesh &, SubMeshInfo &);
  void _fillIndexBuffer(const aiMesh &, SubMeshInfo &);

  [[nodiscard]] const std::vector<SubMeshInfo> &getSubMeshes() const;

  [[nodiscard]] const VertexInfo &getVertexInfo() const;
  [[nodiscard]] int32_t getIndexStride() const;

  [[nodiscard]] std::tuple<const ByteBuffer &, uint64_t> getVertices() const;
  [[nodiscard]] std::tuple<const ByteBuffer &, uint64_t> getIndices() const;
  [[nodiscard]] const AABB &getAABB() const;

private:
  const aiScene *m_scene{nullptr};

  std::vector<SubMeshInfo> m_subMeshes;

  AABB m_aabb{
    .min = glm::vec3{std::numeric_limits<float>::max()},
    .max = glm::vec3{std::numeric_limits<float>::min()},
  };

  VertexInfo m_vertexInfo;
  int32_t m_indexStride{0};

  uint64_t m_numVertices{0};
  ByteBuffer m_vertices;
  uint64_t m_numIndices{0};
  ByteBuffer m_indices;
};
