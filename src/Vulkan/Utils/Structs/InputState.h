#ifndef INPUTSTATE_H
#define INPUTSTATE_H

#include "glm/vec3.hpp"

struct InputState {
  glm::vec3 cameraPos = glm::vec3(0.0f, -8.0f, 2.0f);
  ;
  glm::vec3 cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 cameraRotations = glm::vec3(0.0f, 0.0f, 0.0f);
  float cameraSpeed = 1.0f;
  bool freeCam = true;
  glm::vec3 fixedCamPos = glm::vec3(0.0f, 0.0f, 0.0f);
};

#endif // INPUTSTATE_H