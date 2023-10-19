#pragma once

struct Attributes {
  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
  int numberOfParticles = Utils::PARTICLE_COUNT;
  float smootingLength = 0.12f;
  float mass = 100.0f;
  float stateConstant = 2750.0;
  float polytropicIndex = 1.0f;
  float damping = 200.0f;
  float gravity = 650.0f;
  float timeStep = 0.3f;
  int stop = 1;
};
