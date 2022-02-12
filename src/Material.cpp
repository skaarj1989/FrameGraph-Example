#pragma once

#include "Material.hpp"
#include "Hash.hpp"

namespace std {

template <> struct hash<Material::Parameters> {
  std::size_t operator()(const Material::Parameters &p) const noexcept {
    std::size_t h{0u};
    hashCombine(h, p.shadingModel, p.blendMode, p.cullMode, p.userVertCode,
                p.userFragCode);
    return h;
  }
};

} // namespace std

//
// Material class:
//

ShadingModel Material::getShadingModel() const {
  return m_parameters.shadingModel;
}
BlendMode Material::getBlendMode() const { return m_parameters.blendMode; }
CullMode Material::getCullMode() const { return m_parameters.cullMode; }
TextureResources Material::getDefaultTextures() const {
  return m_parameters.defaultTextures;
}

const std::string_view Material::getUserFragCode() const {
  return m_parameters.userFragCode;
}
const std::string_view Material::getUserVertCode() const {
  return m_parameters.userVertCode;
}

std::size_t Material::getHash() const { return m_hash; }

Material::Material(std::size_t hash, const Parameters &parameters)
    : m_hash{hash}, m_parameters{parameters} {}

//
// Builder:
//

using Builder = Material::Builder;

Builder &Material::Builder::setShadingModel(ShadingModel shadingModel) {
  m_parameters.shadingModel = shadingModel;
  return *this;
}
Builder &Material::Builder::setBlendMode(BlendMode blendMode) {
  m_parameters.blendMode = blendMode;
  return *this;
}
Builder &Material::Builder::setCullMode(CullMode cullMode) {
  m_parameters.cullMode = cullMode;
  return *this;
}
Builder &Material::Builder::addSampler(const std::string &alias,
                                       std::shared_ptr<Texture> texture) {
  m_parameters.defaultTextures[alias] = std::move(texture);
  return *this;
}
Builder &Material::Builder::setUserCode(std::string vert, std::string frag) {
  m_parameters.userVertCode = std::move(vert);
  m_parameters.userFragCode = std::move(frag);
  return *this;
}

std::shared_ptr<Material> Material::Builder::build() {
  const auto h = std::hash<Material::Parameters>{}(m_parameters);
  return std::make_shared<Material>(Material{h, m_parameters});
}
