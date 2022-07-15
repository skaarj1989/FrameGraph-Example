#pragma once

#include "WorldRenderer.hpp"
#include "BasicShapes.hpp"
#include "CubemapConverter.hpp"
#include "ImGuiRenderer.hpp"

#include "MeshCache.hpp"
#include "MaterialCache.hpp"

#include <map>
#include <chrono>

using fsec = std::chrono::duration<float>;

struct GLFWwindow;

class App {
public:
  struct Config {
    const std::string_view caption;
    uint32_t width;
    uint32_t height;
    bool verticalSync{true};
  };

  explicit App(const Config &);
  App(const App &) = delete;
  App(App &&) noexcept = delete;
  ~App();

  App &operator=(const App &) = delete;
  App &operator=(App &&) noexcept = delete;

  void run();

private:
  void _setupUi();

  void _setupScene();

  void _addRenderable(
    const Mesh &, const glm::mat4 &,
    std::optional<std::reference_wrapper<const Material>> materialOverride,
    uint8_t flags = MaterialFlag_Default);

  void _createTower(const glm::ivec3 &dimensions, float spacing,
                    const glm::vec3 &,
                    std::span<std::reference_wrapper<const Material>>);

  void _createSun();
  void _spawnPointLights(uint16_t width, uint16_t depth, float step);

  void _update(fsec dt);
  void _processInput();

  [[nodiscard]] Extent2D _getSwapchainExtent() const;
  [[nodiscard]] glm::vec2 _getMousePosition() const;

  static void _onMouseButton(GLFWwindow *, int button, int action, int mods);
  static void _onMouseScroll(GLFWwindow *, double xoffset, double yoffset);
  static void _onKey(GLFWwindow *, int key, int scancode, int action, int mods);
  static void _onFocus(GLFWwindow *, int focused);
  static void _onCursorEnter(GLFWwindow *, int entered);
  static void _onChar(GLFWwindow *, unsigned int c);

private:
  GLFWwindow *m_window{nullptr};
  bool m_mouseJustPressed[5]{false};

  std::unique_ptr<RenderContext> m_renderContext;

  RenderSettings m_renderSettings;
  std::unique_ptr<WorldRenderer> m_renderer;

  std::unique_ptr<BasicShapes> m_basicShapes;
  std::unique_ptr<CubemapConverter> m_cubemapConverter;
  std::unique_ptr<ImGuiRenderer> m_uiRenderer;

  std::unique_ptr<TextureCache> m_textureCache;
  std::unique_ptr<MaterialCache> m_materialCache;
  std::unique_ptr<MeshCache> m_meshCache;

  AABB m_sceneAABB{.min = glm::vec3{-250.0f}, .max = glm::vec3{250.0f}};

  Texture m_skybox;

  PerspectiveCamera m_camera;
  std::vector<Light> m_lights;
  std::vector<Renderable> m_renderables;
};
