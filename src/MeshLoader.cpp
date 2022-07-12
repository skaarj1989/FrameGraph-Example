#include "MeshLoader.hpp"
#include "MeshImporter.hpp"

#include "glm/gtc/type_ptr.hpp" // make_mat4

namespace {

auto buildMaterial(const MaterialInfo &info, TextureCache &textureCache) {
  Material::Builder builder{};
  builder.setBlendMode(info.blendMode);
  for (auto &[name, path, _] : info.textures) {
    builder.addSampler(name, textureCache.load(path));
  }
  builder.setUserCode("", info.fragCode);

  return builder.build();
}

} // namespace

std::shared_ptr<Mesh> loadMesh(const std::filesystem::path &p,
                               RenderContext &rc, TextureCache &textureCache) {
  Assimp::Importer importer{};
  auto scene = importer.ReadFile(p.string(), 0);

  if (!scene || !scene->mRootNode) {
    throw;
  }

  int32_t flags{aiProcess_Triangulate | aiProcess_ImproveCacheLocality};
  flags |= aiProcess_GenSmoothNormals;
  flags |= aiProcess_CalcTangentSpace;
  flags |= aiProcess_PreTransformVertices;
  // flags |= aiProcess_FlipUVs;
  importer.ApplyPostProcessing(flags);

  MeshImporter meshImporter{scene};

  VertexFormat::Builder builder{};
  for (const auto &[location, attribute] :
       meshImporter.getVertexInfo().getAttributes()) {
    builder.setAttribute(location, attribute);
  }
  auto vertexFormat = builder.build();

  auto [vertices, numVertices] = meshImporter.getVertices();
  auto vertexBuffer = rc.createVertexBuffer(vertexFormat->getStride(),
                                            numVertices, vertices.data());
  auto [indices, numIndices] = meshImporter.getIndices();
  auto indexBuffer = rc.createIndexBuffer(
    IndexType(meshImporter.getIndexStride()), numIndices, indices.data());

  std::vector<SubMesh> subMeshes;
  for (auto &sm : meshImporter.getSubMeshes()) {
    subMeshes.push_back(
      {sm.geometryInfo, buildMaterial(sm.materialInfo, textureCache)});
  }

  return std::make_shared<Mesh>(Mesh{
    .vertexFormat = vertexFormat,
    .vertexBuffer =
      std::shared_ptr<VertexBuffer>(new VertexBuffer{std::move(vertexBuffer)},
                                    RenderContext::ResourceDeleter{rc}),
    .indexBuffer =
      std::shared_ptr<IndexBuffer>(new IndexBuffer{std::move(indexBuffer)},
                                   RenderContext::ResourceDeleter{rc}),
    .subMeshes = std::move(subMeshes),
    .aabb = meshImporter.getAABB(),
  });
}
