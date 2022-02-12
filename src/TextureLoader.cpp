#include "TextureLoader.hpp"
#include "RenderContext.hpp"
#include "Math.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::shared_ptr<Texture> loadTexture(const std::filesystem::path &p,
                                     RenderContext &rc) {
  stbi_set_flip_vertically_on_load(true);

  auto f = stbi__fopen(p.string().c_str(), "rb");
  assert(f);

  const auto hdr = stbi_is_hdr_from_file(f);

  int32_t width, height, numChannels;
  auto pixels =
    hdr ? (void *)stbi_loadf_from_file(f, &width, &height, &numChannels, 0)
        : (void *)stbi_load_from_file(f, &width, &height, &numChannels, 0);
  fclose(f);
  assert(pixels);

  ImageData imageData{
    .dataType = static_cast<GLenum>(hdr ? GL_FLOAT : GL_UNSIGNED_BYTE),
    .pixels = pixels,
  };
  PixelFormat pixelFormat{PixelFormat::Unknown};
  switch (numChannels) {
  case 1:
    imageData.format = GL_RED;
    pixelFormat = PixelFormat::R8_UNorm;
    break;
  case 3:
    imageData.format = GL_RGB;
    pixelFormat = hdr ? PixelFormat::RGB16F : PixelFormat::RGB8_UNorm;
    break;
  case 4:
    imageData.format = GL_RGBA;
    pixelFormat = hdr ? PixelFormat::RGBA16F : PixelFormat::RGBA8_UNorm;
    break;

  default:
    assert(false);
  }

  uint32_t numMipLevels{1u};
  if (isPowerOf2(width) and isPowerOf2(height))
    numMipLevels = calcMipLevels(glm::max(width, height));

  auto texture = rc.createTexture2D(
    {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, pixelFormat,
    numMipLevels);
  rc.upload(texture, 0, {width, height}, imageData)
    .setupSampler(texture, {
                             .minFilter = TexelFilter::Linear,
                             .mipmapMode = MipmapMode::Linear,
                             .magFilter = TexelFilter::Linear,
                           });
  stbi_image_free(pixels);

  if (numMipLevels > 1) rc.generateMipmaps(texture);
  return std::make_shared<Texture>(std::move(texture));
}
