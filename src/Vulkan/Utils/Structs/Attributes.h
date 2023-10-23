#pragma once

struct Attributes {
  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
  int numberOfParticles = Utils::PARTICLE_COUNT;
  float smootingLength = 0.12f;
  float mass = 100.0f;
  float stateConstant = 3000.0f;
  float polytropicIndex = 0.990f;
  float damping = 0.00035f;
  float dampingAcceleration = 0.2726f;
  float dampingVelocity = 0.0f;
  float gravity = 4.55f;
  float timeStep = 1.0f;
  int stop = 1;
  glm::vec2 pad;
};
