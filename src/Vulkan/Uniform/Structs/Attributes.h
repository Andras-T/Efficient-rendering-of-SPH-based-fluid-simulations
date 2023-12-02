#pragma once

struct Attributes {
  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
  int numberOfParticles = Utils::PARTICLE_COUNT;
  float smootingLength = 0.115f;
  float mass = 0.1f;
  float stateConstant = 0.195f;
  float polytropicIndex = 2.4f;
  float damping = 0.0004f;
  float dampingAcceleration = 0.35f;
  float dampingVelocity = 0.0006f;
  float gravity = 20.0f + 7.5f;
  float timeStep = 0.275f;
  int stop = 1;
  glm::vec2 pad;
};
