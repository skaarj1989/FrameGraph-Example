#include "PerspectiveCamera.hpp"
#include "glm/gtc/matrix_transform.hpp"

PerspectiveCamera &PerspectiveCamera::setPerspective(float fov,
                                                     float aspectRatio,
                                                     float zNear, float zFar) {
  if (!glm::isnan(aspectRatio)) {
    m_projectionMatrix =
      glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);

    m_aspectRatio = aspectRatio;
    m_fov = fov;
    m_near = zNear;
    m_far = zFar;
  }
  return *this;
}

PerspectiveCamera &PerspectiveCamera::setPosition(const glm::vec3 &v) {
  m_position = v;
  return *this;
}
PerspectiveCamera &PerspectiveCamera::setPitch(float s) {
  m_pitch = s;
  return *this;
}
PerspectiveCamera &PerspectiveCamera::setYaw(float s) {
  m_yaw = s;
  return *this;
}
PerspectiveCamera &PerspectiveCamera::setRoll(float s) {
  m_roll = s;
  return *this;
}

void PerspectiveCamera::update() {
  const glm::vec3 forward{
    glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch)),
    glm::sin(glm::radians(m_pitch)),
    glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch))};

  m_forward = glm::normalize(forward);
  m_right = glm::normalize(glm::cross(m_forward, {0.0f, 1.0f, 0.0f}));
  m_up = glm::normalize(glm::cross(m_right, m_forward));

  m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
  m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
  m_frustum.update(m_viewProjectionMatrix);
}

const Frustum &PerspectiveCamera::getFrustum() const { return m_frustum; }

float PerspectiveCamera::getFov() const { return m_fov; }
float PerspectiveCamera::getAspectRatio() const { return m_aspectRatio; }
float PerspectiveCamera::getNear() const { return m_near; }
float PerspectiveCamera::getFar() const { return m_far; }

const glm::vec3 &PerspectiveCamera::getPosition() const { return m_position; }

float PerspectiveCamera::getPitch() const { return m_pitch; }
float PerspectiveCamera::getYaw() const { return m_yaw; }
float PerspectiveCamera::getRoll() const { return m_roll; }

const glm::vec3 &PerspectiveCamera::getRight() const { return m_right; }
const glm::vec3 &PerspectiveCamera::getUp() const { return m_up; }
const glm::vec3 &PerspectiveCamera::getForward() const { return m_forward; }

const glm::mat4 &PerspectiveCamera::getView() const { return m_viewMatrix; }
const glm::mat4 &PerspectiveCamera::getProjection() const {
  return m_projectionMatrix;
}
const glm::mat4 &PerspectiveCamera::getViewProjection() const {
  return m_viewProjectionMatrix;
}
