#pragma once

#include "glm/vec3.hpp"

struct InputState {
  glm::vec3 cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 cameraRotations = glm::vec3(0.0f, 0.0f, 0.0f);
  float cameraSpeed = 0.05f;
  bool freeCam = false;
  glm::vec3 fixedCamPos = glm::vec3(0.12f, -0.17, 0.025f);
  glm::vec3 fixedCamTarget = glm::vec3(0.0f, 0.0f, -0.02f);
  glm::vec3 cameraPos = fixedCamPos;// glm::vec3(0.0, -0.021f, 0.0);
  float far = 2.0f;
  float near = 0.092f;
};
