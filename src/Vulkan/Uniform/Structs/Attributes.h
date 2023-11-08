#pragma once

struct Attributes {
  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
  int numberOfParticles = Utils::PARTICLE_COUNT;
  float smootingLength = 0.115f;
  float mass = 300.0f;
  float stateConstant = 3000.0f + 700.0f;
  float polytropicIndex = 0.990f;
  float damping = 0.0004f;
  float dampingAcceleration = 0.3f;
  float dampingVelocity = 0.0002f;
  float gravity = 20.0f + 7.5f;
  float timeStep = 0.275f;
  int stop = 1;
  glm::vec2 pad;
};
