#include "App.hpp"
#include "GLFW/glfw3.h"
#include "Math.hpp"
#include "LightUtility.hpp"

#include "TextureLoader.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "spdlog/spdlog.h"

#include "Tracy.hpp"
#include "TracyOpenGL.hpp"

#include <random>
#include <array>

namespace {

const std::filesystem::path kAssetsDir{"../assets/"};

void showMetricsOverlay() {
  const auto windowFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

  const ImVec2 pad{10.0f, 10.0f};
  const auto *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos + pad, ImGuiCond_Always);

  ImGui::SetNextWindowBgAlpha(0.35f);
  if (ImGui::Begin("MetricsOverlay", nullptr, windowFlags)) {
    const auto frameRate = ImGui::GetIO().Framerate;
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / frameRate, frameRate);
  }
  ImGui::End();
}
void renderSettingsWidget(RenderSettings &settings) {
  if (ImGui::Begin("RenderSettings")) {
    ImGui::Combo("OutputMode",
                 reinterpret_cast<int32_t *>(&settings.outputMode),
                 "Depth\0Emissive\0BaseColor\0Normal\0Metallic\0Roughness\0"
                 "AmbientOcclusion\0SSAO\0BrightColor\0Reflections\0Accum\0"
                 "Reveal\0LightHeatmap\0HDR\0FinalImage");

    ImGui::Text("Features:");
    ImGui::CheckboxFlags("Shadows", &settings.renderFeatures,
                         RenderFeature_Shadows);

    ImGui::CheckboxFlags("SSAO", &settings.renderFeatures, RenderFeature_SSAO);
    ImGui::CheckboxFlags("SSR", &settings.renderFeatures, RenderFeature_SSR);

    ImGui::Separator();

    ImGui::CheckboxFlags("Bloom", &settings.renderFeatures,
                         RenderFeature_Bloom);
    ImGui::SliderFloat("Threshold", &settings.bloom.threshold, 0.1f, 10.0f);
    ImGui::SliderInt("NumPasses", &settings.bloom.numPasses, 1, 10);
    constexpr auto kMaxBlurScale = 4.5f;
    ImGui::SliderFloat("BlurScale", &settings.bloom.blurScale, 0.1f,
                       kMaxBlurScale);

    ImGui::Separator();

    ImGui::CheckboxFlags("FXAA", &settings.renderFeatures, RenderFeature_FXAA);

    ImGui::CheckboxFlags("Vignette", &settings.renderFeatures,
                         RenderFeature_Vignette);

    ImGui::Separator();

    ImGui::Combo("Tonemap", reinterpret_cast<int32_t *>(&settings.tonemap),
                 "Clamp\0ACES\0Filmic\0Reinhard\0Uncharted\0");

    ImGui::Separator();

    ImGui::Text("DebugFlags:");
    ImGui::CheckboxFlags("Wireframe", &settings.debugFlags,
                         DebugFlag_Wireframe);
    ImGui::CheckboxFlags("Visualize Cascade Splits", &settings.debugFlags,
                         DebugFlag_CascadeSplits);
  }
  ImGui::End();
}

void cameraController(PerspectiveCamera &camera, const ImVec2 &mouseDelta,
                      bool mouseButtons[]) {
  constexpr auto LMB = 0, RMB = 1;
  constexpr auto kMouseSensitivity = 0.12f;
  constexpr auto kMoveSensitivity = kMouseSensitivity * 0.2f;

  auto position = camera.getPosition();
  auto yaw = camera.getYaw();
  auto pitch = camera.getPitch();

  if (mouseButtons[LMB] && mouseButtons[RMB]) {
    // Zoom on LMB + RMB
    position -= camera.getForward() * (mouseDelta.x * kMoveSensitivity);
    position += camera.getForward() * (mouseDelta.y * kMoveSensitivity);
  } else {
    if (mouseButtons[LMB]) {
      // Rotate on LMB
      yaw += mouseDelta.x * kMouseSensitivity;
      pitch -= mouseDelta.y * kMouseSensitivity;

      if (pitch > 89.f) pitch = 89.f;
      if (pitch < -89.f) pitch = -89.f;
    } else if (mouseButtons[RMB]) {
      // Pan on RMB
      position -= camera.getRight() * (mouseDelta.x * kMoveSensitivity);
      position += camera.getUp() * (mouseDelta.y * kMoveSensitivity);
    }
  }
  camera.setPosition(position).setYaw(yaw).setPitch(pitch).update();
}

[[nodiscard]] auto getRandomOf(std::default_random_engine &gen,
                               auto container) {
  std::uniform_int_distribution<std::size_t> d{0u, container.size() - 1u};
  return *std::next(container.begin(), d(gen));
}

} // namespace

//
// App class:
//

App::App(const Config &config) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

  m_window = glfwCreateWindow(config.width, config.height,
                              config.caption.data(), nullptr, nullptr);

  glfwSetWindowUserPointer(m_window, this);
  glfwSetWindowFocusCallback(m_window, _onFocus);
  glfwSetCursorEnterCallback(m_window, _onCursorEnter);
  glfwSetMouseButtonCallback(m_window, _onMouseButton);
  glfwSetScrollCallback(m_window, _onMouseScroll);
  glfwSetKeyCallback(m_window, _onKey);
  glfwSetCharCallback(m_window, _onChar);

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(config.verticalSync ? 1 : 0);

  m_renderContext = std::make_unique<RenderContext>();
  TracyGpuContext;

  m_renderer = std::make_unique<WorldRenderer>(*m_renderContext);
  m_basicShapes = std::make_unique<BasicShapes>(*m_renderContext);
  m_cubemapConverter = std::make_unique<CubemapConverter>(*m_renderContext);

  m_textureCache = std::make_unique<TextureCache>(*m_renderContext);
  m_materialCache = std::make_unique<MaterialCache>(*m_textureCache);
  m_meshCache = std::make_unique<MeshCache>(*m_renderContext, *m_textureCache);

  _setupUi();
  _setupScene();
}
App::~App() {
  m_uiRenderer.reset();
  ImGui::DestroyContext();

  m_basicShapes.reset();
  m_renderer.reset();
  m_renderContext->destroy(m_skybox);
  m_renderContext.reset();

  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void App::run() {
  using namespace std::chrono_literals;
  using clock = std::chrono::high_resolution_clock;

  constexpr fsec kTargetFrameTime{16ms};
  fsec deltaTime{kTargetFrameTime};

  auto &io = ImGui::GetIO();
  ImVec2 lastMousePos{0.0f, 0.0f};

  while (!glfwWindowShouldClose(m_window)) {
    const auto beginTicks = clock::now();
    glfwPollEvents();

    io.DeltaTime = deltaTime.count();
    const auto swapchainExtent = _getSwapchainExtent();
    io.DisplaySize = ImVec2{static_cast<float>(swapchainExtent.width),
                            static_cast<float>(swapchainExtent.height)};
    const auto mousePosition = _getMousePosition();
    io.MousePos = ImVec2{mousePosition.x, mousePosition.y};

    for (int32_t i{0}; i < IM_ARRAYSIZE(io.MouseDown); ++i) {
      io.MouseDown[i] =
        m_mouseJustPressed[i] || glfwGetMouseButton(m_window, i) != 0;
      m_mouseJustPressed[i] = false;
    }

    m_camera.setPerspective(
      60.0f, static_cast<float>(swapchainExtent.width) / swapchainExtent.height,
      0.1f, 1000.0f);

    _processInput();
    if (!io.WantCaptureMouse)
      cameraController(m_camera, io.MousePos - lastMousePos, io.MouseDown);
    lastMousePos = io.MousePos;

    _update(deltaTime);

    ImGui::NewFrame();
    showMetricsOverlay();
    renderSettingsWidget(m_renderSettings);

    m_renderer->drawFrame(m_renderSettings, swapchainExtent, m_camera, m_lights,
                          m_renderables, deltaTime.count());

    ImGui::Render();
    m_uiRenderer->draw(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
    TracyGpuCollect;

    deltaTime = clock::now() - beginTicks;
    if (deltaTime > 1s) deltaTime = kTargetFrameTime;

    FrameMark;
  }
}

void App::_setupUi() {
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  auto &io = ImGui::GetIO();

  io.ConfigWindowsMoveFromTitleBarOnly = true;
#if 0
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
#endif
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

  io.Fonts->AddFontDefault();

  m_uiRenderer = std::make_unique<ImGuiRenderer>(*m_renderContext, *io.Fonts);
}

void App::_setupScene() {
  m_camera.setPosition({10.0f, 8.0f, -10.0f}).setPitch(-25.0f).setYaw(135.0f);

  auto equirectangular =
    loadTexture(kAssetsDir / "newport_loft.hdr", *m_renderContext);
  m_skybox = m_cubemapConverter->equirectangularToCubemap(*equirectangular);
  m_renderContext->destroy(*equirectangular);
  m_renderer->setSkybox(m_skybox);

#if 0
  const auto loadMaterial = [&cache =
                               *m_materialCache](const std::string_view name) {
    return cache.load(kAssetsDir /
                      std::format("materials/{0}/{0}.material", name));
  };

  auto stoneBlockWallMaterial = loadMaterial("stone_block_wall");
  _addRenderable(m_basicShapes->getPlane(),
                 glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, -1.5f, 0.0f}),
                 *stoneBlockWallMaterial, MaterialFlag_ReceiveShadow);

  // clang-format off
  const std::vector<std::string> materialNames{
#if 0
    "stringy_marble",
    "streaky_metal",
    "gold_scuffed",
    "rusted_iron",
    "warped_sheet_metal",
    "frosted_glass",
#endif
    "floor_tiles",
  };
  // clang-format on

  std::vector<std::reference_wrapper<const Material>> materials;
  std::transform(
    materialNames.cbegin(), materialNames.cend(),
    std ::back_inserter(materials),
    [loadMaterial](const std::string_view name) -> const Material & {
      return *loadMaterial(name);
    });

  _createTower({3, 3, 3}, 2.5f, {0.0f, -2.5f, 0.0f}, materials);
#else
  auto sponza = m_meshCache->load(kAssetsDir / "meshes/Sponza/Sponza.gltf");
  _addRenderable(*sponza, glm::scale(glm::mat4{1.0f}, glm::vec3{5.0f}), {});
#endif

  _createSun();
  _spawnPointLights(10, 10, 2.0f);
}

void App::_addRenderable(
  const Mesh &mesh, const glm::mat4 &m,
  std::optional<std::reference_wrapper<const Material>> materialOverride,
  uint8_t flags) {

  auto id = 0;
  for (auto &[_, material] : mesh.subMeshes) {
    m_renderables.push_back(Renderable{
      .mesh = mesh,
      .subMeshIndex = id++,
      .material = materialOverride ? materialOverride->get() : *material,
      .flags = flags,
      .modelMatrix = m,
      .aabb = mesh.aabb.transform(m),
    });
  }
}

void App::_createTower(
  const glm::ivec3 &dimensions, float spacing, const glm::vec3 &startPosition,
  std::span<std::reference_wrapper<const Material>> materials) {
  std::random_device rd{};
  std::default_random_engine gen{rd()};

  const auto &mesh = m_basicShapes->getSphere();

  for (int32_t row{0}; row < dimensions.y; ++row)
    for (int32_t col{0}; col < dimensions.x; ++col)
      for (int32_t z{0}; z < dimensions.z; ++z) {
        const glm::vec3 localPosition{(col - (dimensions.x / 2)) * spacing,
                                      row * spacing + spacing,
                                      (z - (dimensions.z / 2)) * spacing};
        auto material = getRandomOf(gen, materials);
        _addRenderable(
          mesh, glm::translate(glm::mat4{1.0}, startPosition + localPosition),
          material);
      }
}

void App::_createSun() {
  m_lights.push_back(Light{
    .type = LightType::Directional,
    .visible = true,
    //.direction = glm::vec3{0.5f, -0.5f, 0.0f},
    .direction = glm::normalize(glm::vec3{0.0f, -1.0f, 0.2f}),
    .color = glm::vec3{0.635f, 0.811f, 0.996f},
    .intensity = 6.0f,
  });
}
void App::_spawnPointLights(uint16_t width, uint16_t depth, float step) {
  std::uniform_real_distribution<float> dist{0.0f, 1.0f};
  std::random_device rd{};
  std::default_random_engine gen{rd()};

  for (int32_t x{-width}; x <= width; x += step) {
    for (int32_t z{-depth}; z <= depth; z += step) {
      const glm::vec3 position{x, -0.5f, z};
      const glm::vec3 color{dist(gen), dist(gen), dist(gen)};
      const auto range = calculateLightRadius(color);
      m_lights.emplace_back(Light{
        .type = LightType::Point,
        .position = position,
        .color = color,
        .intensity = range * 2.0f,
        .range = range,
        .castsShadow = false,
      });
    }
  }
}

void App::_update(fsec dt) {}
void App::_processInput() {
  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(m_window, true);
}

Extent2D App::_getSwapchainExtent() const {
  int32_t w, h;
  glfwGetFramebufferSize(m_window, &w, &h);
  return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}
glm::vec2 App::_getMousePosition() const {
  double x, y;
  glfwGetCursorPos(m_window, &x, &y);
  return {static_cast<float>(x), static_cast<float>(y)};
}

void App::_onMouseButton(GLFWwindow *window, int button, int action, int mods) {
  auto app = static_cast<App *>(glfwGetWindowUserPointer(window));
  if (action == GLFW_PRESS && button >= 0 &&
      button < IM_ARRAYSIZE(m_mouseJustPressed))
    app->m_mouseJustPressed[button] = true;
}
void App::_onMouseScroll(GLFWwindow *window, double xoffset, double yoffset) {
  auto &io = ImGui::GetIO();
  io.MouseWheelH += static_cast<float>(xoffset);
  io.MouseWheel += static_cast<float>(yoffset);
}
void App::_onKey(GLFWwindow *window, int key, int scancode, int action,
                 int mods) {
  auto &io = ImGui::GetIO();
  if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown)) {
    if (action == GLFW_PRESS) {
      io.KeysDown[key] = true;
    }
    if (action == GLFW_RELEASE) {
      io.KeysDown[key] = false;
    }
  }

  // Modifiers are not reliable across systems
  io.KeyCtrl =
    io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  io.KeyShift =
    io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
  io.KeySuper = false;
#else
  io.KeySuper =
    io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}
void App::_onFocus(GLFWwindow *window, int focused) {
  auto &io = ImGui::GetIO();
  io.AddFocusEvent(focused != 0);
}
void App::_onCursorEnter(GLFWwindow *window, int entered) {}
void App::_onChar(GLFWwindow *window, unsigned int c) {
  auto &io = ImGui::GetIO();
  io.AddInputCharacter(c);
}
