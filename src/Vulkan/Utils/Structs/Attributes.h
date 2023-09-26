#pragma once

struct Attributes {
  glm::vec4 center = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  float smootingLength = 0.105f;
  float mass = 25.0f;
  float stateConstant = 225.0f; // equation of state constant
  float polytropicIndex = 1.0f;
  float damping = 30.0f;
  float gravity = 150.0f;
  int stop = 1;
  float pad;
};
