#include "UploadFrameBlock.hpp"

#include "fg/FrameGraph.hpp"
#include "fg/Blackboard.hpp"

#include "FrameGraphBuffer.hpp"
#include "FrameGraphHelper.hpp"

#include "FrameData.hpp"

#include "TracyOpenGL.hpp"

namespace {

struct alignas(16) GPUCamera {
  GPUCamera(const PerspectiveCamera &camera)
      : projection{camera.getProjection()},
        inversedProjection{glm::inverse(projection)}, view{camera.getView()},
        inversedView{glm::inverse(view)}, fov{camera.getFov()},
        near{camera.getNear()}, far{camera.getFar()} {}

private:
  glm::mat4 projection{1.0f};
  glm::mat4 inversedProjection{1.0f};
  glm::mat4 view{1.0f};
  glm::mat4 inversedView{1.0f};
  float fov;
  float near, far;
  // Implicit padding, 4bytes
};

struct GPUFrameBlock {
  float time{0.0f};
  float deltaTime{0.0f};
  Extent2D resolution{0};
  GPUCamera camera;
  uint32_t renderFeatures{0};
  uint32_t debugFlags{0};
};

} // namespace

void uploadFrameBlock(FrameGraph &fg, FrameGraphBlackboard &blackboard,
                      const FrameInfo &frameInfo) {
  blackboard.add<FrameData>() = fg.addCallbackPass<FrameData>(
    "UploadFrameBlock",
    [&](FrameGraph::Builder &builder, FrameData &data) {
      data.frameBlock = builder.create<FrameGraphBuffer>(
        "FrameBlock", {.size = sizeof(GPUFrameBlock)});
      data.frameBlock = builder.write(data.frameBlock);
    },
    [=](const FrameData &data, FrameGraphPassResources &resources, void *ctx) {
      NAMED_DEBUG_MARKER("UploadFrameBlock");
      TracyGpuZone("UploadFrameBlock");

      const GPUFrameBlock frameBlock{
        .time = frameInfo.time,
        .deltaTime = frameInfo.deltaTime,
        .resolution = frameInfo.resolution,
        .camera = GPUCamera{frameInfo.camera},
        .renderFeatures = frameInfo.features,
        .debugFlags = frameInfo.debugFlags,
      };
      static_cast<RenderContext *>(ctx)->upload(
        getBuffer(resources, data.frameBlock), 0, sizeof(GPUFrameBlock),
        &frameBlock);
    });
}
