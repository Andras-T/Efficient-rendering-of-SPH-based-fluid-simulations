#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Structs/InputState.h"
#include "Structs/Particle.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <sstream>
#include <unordered_map>
#include <vector>

class Utils {

private:
  static void createWallParticle(Particle &particle, glm::vec4 position,
                                 glm::vec3 center);

  static void setVertexCount(int c) { vertexCount = c; }

public:
  static constexpr char *ENGINE = "[engine]";
  static constexpr char *VAL_LAYER = "[validation layer]";
  static constexpr char *VULKAN = "[vulkan]";

  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
  static constexpr int MOVABLE_PARTICLE_COUNT = 1000;
  static constexpr int UNMOVABLE_PARTICLE_COUNT = 5292;
  static constexpr float SPHERE_RADIUS = 0.09f;
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

  static void createQuad(std::vector<glm::vec4> &vertices);

  static void createSkyBox(std::vector<glm::vec4> &vertices);

  static glm::mat4 updateCamera(float deltaTime, InputState &inputState,
                                ImGuiIO &io);

  static const std::ostringstream time();
};