#pragma once

#include "Frustum.hpp"

class PerspectiveCamera {
public:
  PerspectiveCamera() = default;
  PerspectiveCamera(const PerspectiveCamera &) = delete;
  PerspectiveCamera(PerspectiveCamera &&) noexcept = default;
  ~PerspectiveCamera() = default;

  PerspectiveCamera &operator=(const PerspectiveCamera &) = delete;
  PerspectiveCamera &operator=(PerspectiveCamera &&) noexcept = default;

  PerspectiveCamera &setPerspective(float fov, float aspectRatio, float zNear,
                                    float zFar);

  PerspectiveCamera &setPosition(const glm::vec3 &);

  PerspectiveCamera &setPitch(float);
  PerspectiveCamera &setYaw(float);
  PerspectiveCamera &setRoll(float);

  void update();

  [[nodiscard]] const Frustum &getFrustum() const;

  float getFov() const;
  float getAspectRatio() const;
  float getNear() const;
  float getFar() const;

  const glm::vec3 &getPosition() const;

  float getPitch() const;
  float getYaw() const;
  float getRoll() const;

  const glm::vec3 &getRight() const;
  const glm::vec3 &getUp() const;
  const glm::vec3 &getForward() const;

  const glm::mat4 &getView() const;
  const glm::mat4 &getProjection() const;
  const glm::mat4 &getViewProjection() const;

private:
  Frustum m_frustum;

  float m_fov{90.0f};
  float m_aspectRatio{1.33f};
  float m_near{0.1f}, m_far{100.0f};

  glm::vec3 m_position{0.0f, 0.0f, 0.0f};
  float m_pitch{0.0f}, m_yaw{0.0f}, m_roll{0.0f};

  glm::vec3 m_right{1.0f, 0.0f, 0.0f};
  glm::vec3 m_up{0.0f, 1.0f, 0.0f};
  glm::vec3 m_forward{0.0f, 0.0f, -1.0f};

  glm::mat4 m_projectionMatrix{1.0f};
  glm::mat4 m_viewMatrix{1.0f};
  glm::mat4 m_viewProjectionMatrix{1.0f};
};
