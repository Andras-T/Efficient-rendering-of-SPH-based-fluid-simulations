#ifndef UNIFORMBUFFEROBJECT_H
#define UNIFORMBUFFEROBJECT_H

#include "glm/gtc/matrix_transform.hpp"

struct UniformBufferObject {
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 proj = glm::mat4(1.0f);
  float deltaTime = 1.0f;
};

#endif // UNIFORMBUFFEROBJECT_H
