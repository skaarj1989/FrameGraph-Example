#include "MaterialLoader.hpp"
#include "RenderContext.hpp"
#include "TextureLoader.hpp"
#include "FileUtility.hpp"
#include "nlohmann/json.hpp"

NLOHMANN_JSON_SERIALIZE_ENUM(ShadingModel,
                             {
                               {ShadingModel::Unlit, "Unlit"},
                               {ShadingModel::DefaultLit, "DefaultLit"},
                             });
NLOHMANN_JSON_SERIALIZE_ENUM(BlendMode,
                             {
                               {BlendMode::Opaque, "Opaque"},
                               {BlendMode::Masked, "Masked"},
                               {BlendMode::Transparent, "Transparent"},
                             });
NLOHMANN_JSON_SERIALIZE_ENUM(CullMode, {
                                         {CullMode::None, "None"},
                                         {CullMode::Front, "Front"},
                                         {CullMode::Back, "Back"},
                                       });

std::shared_ptr<Material> loadMaterial(const std::filesystem::path &p,
                                       RenderContext &rc) {
  const auto j = nlohmann::json::parse(readText(p));

  auto builder = Material::Builder{};
  builder.setShadingModel(j["shadingModel"])
    .setBlendMode(j["blendMode"])
    .setCullMode(j.value("cullMode", CullMode::Back));

  if (j.contains("samplers")) {
    for (const auto &[_, prop] : j["samplers"].items()) {
      const auto texturePath = adjustPath(prop["path"].get<std::string>(), p);
      auto textureResource = loadTexture(texturePath, rc);
      builder.addSampler(prop["name"], textureResource);
    }
  }

  std::string vert;
  if (j.contains("vertexShader")) {
    const auto userCodePath =
      adjustPath(j["vertexShader"].get<std::string>(), p);
    vert = readText(userCodePath);
  }
  std::string frag;
  if (j.contains("fragmentShader")) {
    const auto userCodePath =
      adjustPath(j["fragmentShader"].get<std::string>(), p);
    frag = readText(userCodePath);
  }
  builder.setUserCode(vert, frag);
  return builder.build();
}
