find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

add_executable(
  FrameGraphExample
  "Hash.hpp"
  "Math.hpp"
  "FileUtility.hpp"
  "FileUtility.cpp"
  "ShaderCodeBuilder.hpp"
  "ShaderCodeBuilder.inl"
  "ShaderCodeBuilder.cpp"

  "MeshImporter.hpp"
  "MeshImporter.cpp"
  "ai2glm.hpp"
  "ai2glm.cpp"

  # -- RenderContext:
  "TypeDefs.hpp"
  "Buffer.hpp"
  "Buffer.cpp"
  "VertexBuffer.hpp"
  "VertexBuffer.cpp"
  "IndexBuffer.hpp"
  "IndexBuffer.cpp"
  "VertexAttributes.hpp"
  "VertexAttributes.cpp"
  "Texture.hpp"
  "Texture.cpp"
  "GraphicsPipeline.hpp"
  "GraphicsPipeline.cpp"
  "RenderContext.hpp"
  "RenderContext.cpp"

  # -- FrameGraph:
  "TransientResources.hpp"
  "TransientResources.cpp"
  "FrameGraphTexture.hpp"
  "FrameGraphTexture.cpp"
  "FrameGraphBuffer.hpp"
  "FrameGraphBuffer.cpp"
  "FrameGraphHelper.hpp"
  "FrameGraphHelper.cpp"

  # -- PassData:
  "BRDF.hpp"
  "GlobalLightProbeData.hpp"
  "GBufferData.hpp"
  "WeightedBlendedData.hpp"
  "SSAOData.hpp"
  "ReflectionsData.hpp"
  "LightsData.hpp"
  "LightCullingData.hpp"
  "ShadowMapData.hpp"
  "SceneColorData.hpp"
  "BrightColorData.hpp"

  # -- WorldRenderer:
  "WorldRenderer.hpp"
  "WorldRenderer.cpp"
  "TiledLighting.hpp"
  "TiledLighting.cpp"
  "ShadowRenderer.hpp"
  "ShadowRenderer.cpp"
  "ShadowCascadesBuilder.hpp"
  "ShadowCascadesBuilder.cpp"

  # -- Passes:
  "Passes/BaseGeometryPass.hpp"
  "Passes/BaseGeometryPass.cpp"
  "Passes/GBufferPass.hpp"
  "Passes/GBufferPass.cpp"
  "Passes/WireframePass.hpp"
  "Passes/WireframePass.cpp"
  "Passes/DeferredLightingPass.hpp"
  "Passes/DeferredLightingPass.cpp"
  "Passes/SkyboxPass.hpp"
  "Passes/SkyboxPass.cpp"
  "Passes/WeightedBlendedPass.hpp"
  "Passes/WeightedBlendedPass.cpp"
  "Passes/TransparencyCompositionPass.hpp"
  "Passes/TransparencyCompositionPass.cpp"

  # -- Postprocessing Passes:
  "Passes/Blur.hpp"
  "Passes/Blur.cpp"
  "Passes/SSAO.hpp"
  "Passes/SSAO.cpp"
  "Passes/SSR.hpp"
  "Passes/SSR.cpp"
  "Passes/BrightPass.hpp"
  "Passes/BrightPass.cpp"
  "Passes/TonemapPass.hpp"
  "Passes/TonemapPass.cpp"
  "Passes/FXAA.hpp"
  "Passes/FXAA.cpp"
  "Passes/GammaCorrectionPass.hpp"
  "Passes/GammaCorrectionPass.cpp"
  "Passes/Vignette.hpp"
  "Passes/Vignette.cpp"

  # ---
  "AABB.hpp"
  "AABB.cpp"
  "Sphere.hpp"
  "Cone.hpp"
  "Plane.hpp"
  "Plane.cpp"
  "Frustum.hpp"
  "Frustum.cpp"
  "PerspectiveCamera.hpp"
  "PerspectiveCamera.cpp"
  "VertexFormat.hpp"
  "VertexFormat.cpp"
  "Mesh.hpp"
  "MeshLoader.hpp"
  "MeshLoader.cpp"
  "MeshCache.hpp"
  "MeshCache.cpp"
  "Material.hpp"
  "Material.cpp"
  "MaterialLoader.hpp"
  "MaterialLoader.cpp"
  "MaterialCache.hpp"
  "MaterialCache.cpp"
  "TextureLoader.hpp"
  "TextureLoader.cpp"
  "TextureCache.hpp"
  "TextureCache.cpp"
  "Light.hpp"
  "LightUtility.hpp"
  "LightUtility.cpp"
  "Renderable.hpp"

  # ---
  "BasicShapes.hpp"
  "BasicShapes.cpp"
  "CubeCapture.hpp"
  "CubemapConverter.hpp"
  "CubemapConverter.cpp"
  "IBL.hpp"
  "IBL.cpp"
  "ImGuiRenderer.hpp"
  "ImGuiRenderer.cpp"
  "App.hpp"
  "App.cpp"
  "main.cpp")

# After flipping GLM_FORCE_DEPTH_ZERO_TO_ONE, Recreate project and update
# DEPTH_ZERO_TO_ONE in shaders/Depth.glsl
target_compile_definitions(FrameGraphExample PUBLIC GLM_FORCE_RADIANS)

target_link_libraries(FrameGraphExample
  PRIVATE
  glfw
  glm::glm
  glad::glad
  spdlog::spdlog
  fg::FrameGraph
  nlohmann_json::nlohmann_json
  assimp::assimp
  imgui::imgui
  Tracy::TracyClient
)
