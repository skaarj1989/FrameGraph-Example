#include "MeshImporter.hpp"

#include "assimp/GltfMaterial.h"
#include "assimp/IOSystem.hpp"

#include "ai2glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <filesystem>
#include <numeric> // accumulate
#include <span>

//
// VertexInfo class:
//

uint32_t VertexInfo::getStride() const { return m_stride; }
const VertexAttribute &
VertexInfo::getAttribute(AttributeLocation location) const {
  return m_attributes.at(location);
}
const AttributesMap &VertexInfo::getAttributes() const { return m_attributes; }

VertexInfo::VertexInfo(int32_t stride, AttributesMap &&attributes)
    : m_stride{stride}, m_attributes{std::move(attributes)} {}

using Builder = VertexInfo::Builder;

Builder &Builder::add(AttributeLocation location, VertexAttribute::Type type) {
  if (!m_attributes.contains(location)) {
    m_attributes.emplace(location, VertexAttribute{
                                     .type = type,
                                     .offset = m_currentOffset,
                                   });
    m_currentOffset += getSize(type);
  }
  return *this;
}

VertexInfo Builder::build() {
  return VertexInfo{m_currentOffset, std::move(m_attributes)};
}

//
// MeshImporter class:
//

namespace {

[[nodiscard]] BlendMode getBlendMode(const std::string_view alphaMode) {
  if (alphaMode == "OPAQUE")
    return BlendMode::Opaque;
  else if (alphaMode == "MASK")
    return BlendMode::Masked;
  else if (alphaMode == "BLEND")
    return BlendMode::Transparent;

  assert(false);
  return BlendMode::Opaque;
}
[[nodiscard]] const char *toString(aiTextureType textureType) {
  switch (textureType) {
  case aiTextureType_DIFFUSE:
  case aiTextureType_BASE_COLOR:
    return "BaseColor";
  case aiTextureType_SPECULAR:
    return "Specular";
  case aiTextureType_AMBIENT:
    return "Ambient";
  case aiTextureType_EMISSIVE:
  case aiTextureType_EMISSION_COLOR:
    return "Emissive";
  case aiTextureType_HEIGHT:
    return "Height";
  case aiTextureType_NORMALS:
    return "Normals";
  case aiTextureType_SHININESS:
    return "Shininess";
  case aiTextureType_OPACITY:
    return "Opacity";
  case aiTextureType_DISPLACEMENT:
    return "Displacement";
  case aiTextureType_LIGHTMAP:
  case aiTextureType_AMBIENT_OCCLUSION:
    return "AmbientOcclusion";
  case aiTextureType_REFLECTION:
    return "Reflection";
  case aiTextureType_METALNESS:
    return "Metallic";
  case aiTextureType_DIFFUSE_ROUGHNESS:
    return "Roughness";
  case aiTextureType_TRANSMISSION:
    return "Transmission";
  case aiTextureType_UNKNOWN:
    return "MetallicRoughness";
  }
  return "Unknown";
}

[[nodiscard]] uint64_t countIndices(const aiMesh &mesh) {
  return std::accumulate(mesh.mFaces, mesh.mFaces + mesh.mNumFaces, 0,
                         [](const auto count, const auto &face) {
                           return count + face.mNumIndices;
                         });
}

[[nodiscard]] int32_t findIndexStride(uint64_t numIndices) {
  if (numIndices <= UINT16_MAX)
    return sizeof(uint16_t);
  else if (numIndices <= UINT32_MAX)
    return sizeof(uint32_t);

  assert(false);
  return 0;
}

} // namespace

MeshImporter::MeshImporter(const aiScene *scene) : m_scene{scene} {
  _findVertexFormat();
  _processMeshes();
}

void MeshImporter::_findVertexFormat() {
  auto builder = VertexInfo::Builder{};
  builder.add(AttributeLocation::Position, VertexAttribute::Type::Float3);
  for (const auto *mesh : std::span{m_scene->mMeshes, m_scene->mNumMeshes}) {
    if (mesh->GetNumColorChannels() > 0)
      builder.add(AttributeLocation::Color_0, VertexAttribute::Type::Float4);

    if (mesh->HasNormals())
      builder.add(AttributeLocation::Normal, VertexAttribute::Type::Float3);

    if (const auto numTexCoords = mesh->GetNumUVChannels(); numTexCoords > 0) {
      builder.add(AttributeLocation::TexCoord_0, VertexAttribute::Type::Float2);
      if (mesh->HasTangentsAndBitangents()) {
        builder.add(AttributeLocation::Tangent, VertexAttribute::Type::Float3);
        builder.add(AttributeLocation::Bitangent,
                    VertexAttribute::Type::Float3);
      }
      if (numTexCoords > 1) {
        builder.add(AttributeLocation::TexCoord_1,
                    VertexAttribute::Type::Float2);
      }
    }

    m_numVertices += mesh->mNumVertices;
    for (const auto &face : std::span{mesh->mFaces, mesh->mNumFaces}) {
      m_numIndices += face.mNumIndices;
    }
  }

  m_vertexInfo = builder.build();
  m_vertices.reserve(m_numVertices * m_vertexInfo.getStride());

  m_indexStride = findIndexStride(m_numIndices);
  m_indices.reserve(m_numIndices * m_indexStride);
}

void MeshImporter::_processMeshes() {
  for (const auto *mesh : std::span{m_scene->mMeshes, m_scene->mNumMeshes}) {
    const auto &subMesh = _processMesh(*mesh);

    if (subMesh.aabb.min.x < m_aabb.min.x) m_aabb.min.x = subMesh.aabb.min.x;
    if (subMesh.aabb.min.y < m_aabb.min.y) m_aabb.min.y = subMesh.aabb.min.y;
    if (subMesh.aabb.min.z < m_aabb.min.z) m_aabb.min.z = subMesh.aabb.min.z;

    if (subMesh.aabb.max.x > m_aabb.max.x) m_aabb.max.x = subMesh.aabb.max.x;
    if (subMesh.aabb.max.y > m_aabb.max.y) m_aabb.max.y = subMesh.aabb.max.y;
    if (subMesh.aabb.max.z > m_aabb.max.z) m_aabb.max.z = subMesh.aabb.max.z;
  }
}

SubMeshInfo MeshImporter::_processMesh(const aiMesh &mesh) {
  auto &info = m_subMeshes.emplace_back(SubMeshInfo{
    .name = mesh.mName.C_Str(),
    .materialInfo = _processMaterial(mesh.mMaterialIndex,
                                     *m_scene->mMaterials[mesh.mMaterialIndex]),
  });

  _fillVertexBuffer(mesh, info);
  _fillIndexBuffer(mesh, info);
  return info;
}
MaterialInfo MeshImporter::_processMaterial(uint32_t id,
                                            const aiMaterial &material) {
  MaterialInfo materialInfo;
  if (aiString alphaMode;
      material.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
    materialInfo.blendMode = getBlendMode(alphaMode.C_Str());
  }

  auto fetchTextureInfo =
    [&material](aiTextureType type) -> std::optional<TextureInfo> {
    if (auto count = material.GetTextureCount(type); count == 0)
      return std::nullopt;

    aiString path;
    uint32_t uvIndex{0};
    material.GetTexture(type, 0, &path, nullptr, &uvIndex);
    std::filesystem::path p{path.C_Str()};

    return TextureInfo{
      .name = std::string("t_") + toString(type),
      .path = p.generic_string(),
    };
  };

  std::vector<std::string> defines;

  if (auto baseColor = fetchTextureInfo(aiTextureType_BASE_COLOR); baseColor) {
    defines.push_back("HAS_BASE_COLOR");
    materialInfo.textures.emplace(*baseColor);
  }
  if (auto normal = fetchTextureInfo(aiTextureType_NORMALS)) {
    defines.push_back("HAS_NORMAL_MAP");
    materialInfo.textures.emplace(*normal);
  }
  if (auto metalRoughAO = fetchTextureInfo(aiTextureType_UNKNOWN)) {
    defines.push_back("HAS_METAL_ROUGH_AO_MAP");
    materialInfo.textures.emplace(*metalRoughAO);
  }

  std::ostringstream oss;
  std::transform(defines.cbegin(), defines.cend(),
                 std::ostream_iterator<std::string>{oss, "\n"},
                 [](const auto &s) { return std::format("#define {}", s); });

  oss << R"(
const vec2 texCoord = getTexCoord0();

#ifdef HAS_BASE_COLOR
const vec4 baseColor = texture(t_BaseColor, texCoord);
material.baseColor.rgb = sRGBToLinear(baseColor.rgb);

# if BLEND_MODE == BLEND_MODE_MASKED
material.visible = baseColor.a > 0.7;
# endif
#endif

#ifdef HAS_NORMAL_MAP
const vec3 N = sampleNormalMap(t_Normals, texCoord);
material.normal = tangentToWorld(N, texCoord);
#endif

#ifdef HAS_METAL_ROUGH_AO_MAP
const vec3 temp = texture(t_MetallicRoughness, texCoord).rgb;
material.metallic = temp.b;
material.roughness = temp.g;
material.ambientOcclusion = temp.r; 
#endif
)";

  materialInfo.fragCode = oss.str();

  return materialInfo;
}

void MeshImporter::_fillVertexBuffer(const aiMesh &mesh, SubMeshInfo &info) {
  const auto stride = m_vertexInfo.getStride();
  info.geometryInfo.vertexOffset =
    m_vertices.size() > 0 ? m_vertices.size() / stride : 0;
  info.geometryInfo.numVertices = mesh.mNumVertices;

  m_vertices.insert(m_vertices.cend(), info.geometryInfo.numVertices * stride,
                    std::byte{0});
  auto currentVertex =
    m_vertices.data() + (info.geometryInfo.vertexOffset * stride);

  const auto copySlice = [&](AttributeLocation location, const auto &value,
                             std::size_t size, int32_t offset = 0) {
    const auto &attrib = m_vertexInfo.getAttribute(location);
    assert(size == getSize(attrib.type));
    memcpy(currentVertex + attrib.offset + offset, glm::value_ptr(value), size);
  };

  const auto rootTransform = m_scene->mRootNode->mTransformation;
  const auto normalMatrix =
    aiMatrix3x3{aiMatrix4x4{rootTransform}.Inverse().Transpose()};

  aiVector3D min{std::numeric_limits<float>::max()};
  aiVector3D max{std::numeric_limits<float>::min()};

  for (uint32_t i{0}; i < mesh.mNumVertices; ++i) {
    const auto pos = rootTransform * mesh.mVertices[i];
    copySlice(AttributeLocation::Position, to_vec3(pos), sizeof(glm::vec3));

    if (pos.x < min.x) min.x = pos.x;
    if (pos.y < min.y) min.y = pos.y;
    if (pos.z < min.z) min.z = pos.z;
    if (pos.x > max.x) max.x = pos.x;
    if (pos.y > max.y) max.y = pos.y;
    if (pos.z > max.z) max.z = pos.z;

    if (mesh.GetNumColorChannels() > 0) {
      copySlice(AttributeLocation::Color_0, to_vec4(mesh.mColors[0][i]),
                sizeof(glm::vec4));
    }
    if (mesh.HasNormals()) {
      copySlice(AttributeLocation::Normal,
                to_vec3((normalMatrix * mesh.mNormals[i]).Normalize()),
                sizeof(glm::vec3));
    }
    if (mesh.mTextureCoords[0]) {
      copySlice(AttributeLocation::TexCoord_0,
                to_vec2(mesh.mTextureCoords[0][i]), sizeof(glm::vec2));
      if (mesh.HasTangentsAndBitangents()) {
        copySlice(AttributeLocation::Tangent,
                  to_vec3((normalMatrix * mesh.mTangents[i]).Normalize()),
                  sizeof(glm::vec3));
        copySlice(AttributeLocation::Bitangent,
                  to_vec3((normalMatrix * mesh.mBitangents[i]).Normalize()),
                  sizeof(glm::vec3));
      }
    }
    if (mesh.mTextureCoords[1]) {
      copySlice(AttributeLocation::TexCoord_1,
                to_vec2(mesh.mTextureCoords[1][i]), sizeof(glm::vec2));
    }

    currentVertex += stride;
  }

  info.aabb.min = to_vec3(min);
  info.aabb.max = to_vec3(max);
}

void MeshImporter::_fillIndexBuffer(const aiMesh &mesh, SubMeshInfo &info) {
  info.geometryInfo.indexOffset =
    m_indices.size() > 0 ? m_indices.size() / m_indexStride : 0u;
  info.geometryInfo.numIndices = countIndices(mesh);

  m_indices.insert(m_indices.cend(),
                   info.geometryInfo.numIndices * m_indexStride, std::byte{0});
  auto currentIndex =
    m_indices.data() + (info.geometryInfo.indexOffset * m_indexStride);

  for (const auto &face : std::span{mesh.mFaces, mesh.mNumFaces})
    for (const auto index : std::span{face.mIndices, face.mNumIndices}) {
      memcpy(currentIndex, &index, m_indexStride);
      currentIndex += m_indexStride;
    }
}

const std::vector<SubMeshInfo> &MeshImporter::getSubMeshes() const {
  return m_subMeshes;
}

const VertexInfo &MeshImporter::getVertexInfo() const { return m_vertexInfo; }
int32_t MeshImporter::getIndexStride() const { return m_indexStride; }

std::tuple<const ByteBuffer &, uint64_t> MeshImporter::getVertices() const {
  return {m_vertices, m_numVertices};
}
std::tuple<const ByteBuffer &, uint64_t> MeshImporter::getIndices() const {
  return {m_indices, m_numIndices};
}
const AABB &MeshImporter::getAABB() const { return m_aabb; }
