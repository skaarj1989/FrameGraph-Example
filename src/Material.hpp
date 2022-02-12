#pragma once

#include "GraphicsPipeline.hpp"
#include "Texture.hpp"

#include <memory>
#include <string>
#include <map>

enum class ShadingModel { Unlit = 0, DefaultLit };
enum class BlendMode {
  Opaque = 0,
  Masked,
  Transparent,
};

enum MaterialFlag_ {
  MaterialFlag_None = 0,
  MaterialFlag_CastShadow = 1 << 1,
  MaterialFlag_ReceiveShadow = 1 << 2,

  MaterialFlag_Default = MaterialFlag_CastShadow | MaterialFlag_ReceiveShadow
};

using TextureResources = std::map<std::string, std::shared_ptr<Texture>>;

class Material {
public:
  Material(const Material &) = delete;
  Material(Material &&) noexcept = default;
  ~Material() = default;

  Material &operator=(const Material &) = delete;
  Material &operator=(Material &&) noexcept = default;

  [[nodiscard]] ShadingModel getShadingModel() const;
  [[nodiscard]] BlendMode getBlendMode() const;
  [[nodiscard]] CullMode getCullMode() const;
  [[nodiscard]] TextureResources getDefaultTextures() const;

  [[nodiscard]] const std::string_view getUserFragCode() const;
  [[nodiscard]] const std::string_view getUserVertCode() const;

  [[nodiscard]] std::size_t getHash() const;

  struct Parameters {
    ShadingModel shadingModel{ShadingModel::DefaultLit};
    BlendMode blendMode{BlendMode::Opaque};
    CullMode cullMode{CullMode::Back};
    TextureResources defaultTextures;
    std::string userVertCode, userFragCode;
  };

  class Builder {
  public:
    Builder &setShadingModel(ShadingModel);
    Builder &setBlendMode(BlendMode);
    Builder &setCullMode(CullMode);

    Builder &addSampler(const std::string &alias, std::shared_ptr<Texture>);

    Builder &setUserCode(std::string vert, std::string frag);

    [[nodiscard]] std::shared_ptr<Material> build();

  private:
    Material::Parameters m_parameters;
  };

private:
  Material(std::size_t hash, const Parameters &);

private:
  Parameters m_parameters;
  const std::size_t m_hash{0u};
};
