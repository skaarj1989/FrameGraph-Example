#include "UploadLights.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

#include "FrameGraphBuffer.hpp"
#include "FrameGraphHelper.hpp"

#include "LightsData.hpp"

#include "TracyOpenGL.hpp"

#include "RenderContext.hpp"

#include "glm/vec4.hpp"
#include "glm/common.hpp"

namespace {

// see _LightBuffer in shaders/Lib/Light.glsl
constexpr auto kLightDataOffset = sizeof(uint32_t) * 4;

// - Position and direction in world-space
// - Angles in radians
struct alignas(16) GPULight {
  GPULight(const Light &light) {
    position = glm::vec4{light.position, light.range};
    color = glm::vec4{light.color, light.intensity};
    type = static_cast<uint32_t>(light.type);

    if (light.type == LightType::Spot) {
      innerConeAngle = glm::radians(light.innerConeAngle);
      outerConeAngle = glm::radians(light.outerConeAngle);
    }
    if (light.type != LightType::Point) {
      direction = glm::normalize(glm::vec4{light.direction, 0.0f});
    }
  }

private:
  glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};  // .w = range
  glm::vec4 direction{0.0f, 0.0f, 0.0f, 0.0f}; // .w = unused
  glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};     // .a = intensity
  uint32_t type{0};
  float innerConeAngle{1.0f};
  float outerConeAngle{1.0f};
  // Implicit padding, 4bytes
};

} // namespace

void uploadLights(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                  std::vector<const Light *> &&lights) {
  const auto numLights = uint32_t(lights.size());

  blackboard.add<LightsData>() = fg.addCallbackPass<LightsData>(
    "UploadLights",
    [numLights](FrameGraph::Builder &builder, LightsData &data) {
      const GLsizeiptr bufferSize =
        kLightDataOffset + (sizeof(GPULight) * numLights);
      data.buffer =
        builder.create<FrameGraphBuffer>("LightsBuffer", {.size = bufferSize});
      data.buffer = builder.write(data.buffer);
    },
    [=](const LightsData &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("UploadLights");
      TracyGpuZone("UploadLights");

      auto &rc = *static_cast<RenderContext *>(ctx);

      std::vector<GPULight> gpuLights;
      gpuLights.reserve(lights.size());
      for (const auto *light : lights)
        gpuLights.emplace_back(GPULight{*light});

      auto &buffer = getBuffer(resources, data.buffer);
      rc.upload(buffer, 0, sizeof(uint32_t), &numLights);
      if (numLights > 0) {
        rc.upload(buffer, kLightDataOffset, sizeof(GPULight) * numLights,
                  gpuLights.data());
      }
    });
}
