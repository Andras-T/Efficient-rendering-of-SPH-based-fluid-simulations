#ifndef UTILS_H
#define UTILS_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Structs/InputState.h"
#include "Structs/Particle.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <unordered_map>
#include <vector>

class Utils {

private:
  static void createWallParticle(Particle &particle, glm::vec4 position,
                                 glm::vec4 color, glm::vec3 center);

  static void setVertexCount(int c) { vertexCount = c; }

public:
  static const int MAX_FRAMES_IN_FLIGHT = 2;
  static const int MOVABLE_PARTICLE_COUNT = 1000;
  static const int UNMOVABLE_PARTICLE_COUNT = 5292 + 8;
  static constexpr float SPHERE_RADIUS = 0.065f;
  static const int PARTICLE_COUNT =
      MOVABLE_PARTICLE_COUNT + UNMOVABLE_PARTICLE_COUNT;
  static int vertexCount;

  static int getVertexCount() { return vertexCount; }

  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties,
                                 VkPhysicalDevice &physicalDevice);

  static void initializeParticles(std::vector<Particle> &particles,
                                  glm::vec3 center);

  static void createSphere(std::vector<glm::vec4> &vertices, float radius,
                           int latitudeSegments, int longitudeSegments);

  static glm::mat4 updateCamera(float deltaTime, InputState &inputState,
                                ImGuiIO &io);
};

#endif // UTILS_H
