#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

struct Attributes {
  glm::vec4 center = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  float smootingLength = 0.05f;
  float mass = 10.0f;
  float stateConstant = 1000.0f; // equation of state constant
  float polytropicIndex = 1.0f;
  float damping = 10.0f;
  float gravity = 250.0f;
  int stop = 1;
  float pad;
};

#endif // ATTRIBUTES_H
