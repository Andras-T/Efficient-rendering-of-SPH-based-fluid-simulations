#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H

#include "glm/vec3.hpp"

struct Transformations {
  glm::vec3 scale = glm::vec3(1.0f);
  glm::vec3 rotations = glm::vec3(0.0f);
  glm::vec3 translate = glm::vec3(0.0f);
  glm::vec3 cameraRotations = glm::vec3(0.0f);
  float s = 1.0f;
};

#endif // TRANSFORMATIONS_H
