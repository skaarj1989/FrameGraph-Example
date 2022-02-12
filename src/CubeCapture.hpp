#pragma once

#include "glm/gtc/matrix_transform.hpp"

static const auto kCubeProjection =
  glm::perspective(glm::radians(90.0f), 1.0f, 0.0f, 1.0f);
// clang-format off
static const glm::mat4 kCaptureViews[]{
  glm::lookAt(glm::vec3{0.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}),  // +X
  glm::lookAt(glm::vec3{0.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}),  // -X
  glm::lookAt(glm::vec3{0.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}),  // +Y
  glm::lookAt(glm::vec3{0.0f}, { 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}),  // -Y
  glm::lookAt(glm::vec3{0.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, -1.0f,  0.0f}),  // +Z
  glm::lookAt(glm::vec3{0.0f}, { 0.0f,  0.0f, -1.0f}, {0.0f, -1.0f,  0.0f})}; // -Z
// clang-format on
