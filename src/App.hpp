#pragma once

#include "WorldRenderer.hpp"
#include "BasicShapes.hpp"
#include "CubemapConverter.hpp"
#include "ImGuiRenderer.hpp"

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
  void _loadMaterial(const std::string_view name);

  void _addRenderable(const Mesh &, const glm::vec3 &position,
                      const Material &material,
                      uint8_t flags = MaterialFlag_Default);

  void _createTower(const glm::ivec3 &dimensions, float spacing,
                    const glm::vec3 &,
                    std::span<const std::string> materialNames);

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

  Texture m_skybox;
  std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

  PerspectiveCamera m_camera;
  std::vector<Light> m_lights;
  std::vector<Renderable> m_renderables;
};
