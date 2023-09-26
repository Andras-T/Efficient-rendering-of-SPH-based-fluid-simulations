#include "Utils.h"
#include "imgui_internal.h"
#include <iostream>
#include <stdexcept>

int Utils::vertexCount = 0;

uint32_t Utils::findMemoryType(uint32_t typeFilter,
                               VkMemoryPropertyFlags properties,
                               VkPhysicalDevice &physicalDevice) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void Utils::initializeParticles(std::vector<Particle> &particles,
                                glm::vec3 center) {
  const double side = 1.35;
  const double Volume = side * side * side;
  const double particleVolume = Volume / MOVABLE_PARTICLE_COUNT;
  const double particleLength = pow(particleVolume, 1.0 / 3.0);
  const int perAxis = round(side / particleLength);
  double offset = particleLength / 2 + (2 - side) / 2;
  if (perAxis * particleLength < side)
    offset += (side - perAxis * particleLength) / 2;

  int counter = 0;
  for (size_t i = 0; i < perAxis; i++) {
    for (size_t j = 0; j < perAxis; j++) {
      for (int k = 0; k < perAxis; k++) {
        float x = (offset + k * particleLength) - 1;
        float y = (offset + j * particleLength) - 1;
        float z = (offset + i * particleLength) - 1;
        particles[counter].position = glm::vec4(glm::vec3(x, y, z) + center, 1);
        particles[counter].color = glm::vec4(0.15, 0.25, 1.0, 0.3);
        particles[counter].velocity = glm::vec3(0, 0, 0);
        particles[counter].movable = 1.0f;
        particles[counter].acc = glm::vec3(0.0, 0.0, 0.0);
        counter++;
      }
    }
  }

  if (counter < MOVABLE_PARTICLE_COUNT || counter > MOVABLE_PARTICLE_COUNT)
    std::cerr << "Failed to initialize the partciles"
              << MOVABLE_PARTICLE_COUNT - counter << " counter: " << counter;

  const float dist_from_each = 0.075f;
  glm::vec4 insideWallColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  glm::vec4 outsideWallColor = insideWallColor;
  glm::vec4 testColor(1, 0, 0, 0.02);

  float lowest = -0.75;
  float highest = 0.75;

  int c = 0;
  for (float y = lowest; y <= highest + 0.001f; y += 0.075f) {
    for (float z = lowest; z <= highest + 0.001f; z += 0.075f) {
      ++c;
    }
  }
  std::cout << "engine: Number of wall particles: " << c * 12 << std::endl;

  for (float y = lowest; y <= highest + 0.001f; y += dist_from_each) {
    for (float z = lowest; z <= highest + 0.001f; z += dist_from_each) {
      createWallParticle(particles[counter++], glm::vec4(lowest, y, z, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(lowest + dist_from_each / 2.0f, y, z, 1.0f),
                         insideWallColor, center);

      createWallParticle(particles[counter++], glm::vec4(highest, y, z, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(highest - dist_from_each / 2.0f, y, z, 1.0f),
                         insideWallColor, center);

      createWallParticle(particles[counter++], glm::vec4(y, lowest, z, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(y, lowest + dist_from_each / 2.0f, z, 1.0f),
                         insideWallColor, center);

      createWallParticle(particles[counter++], glm::vec4(y, highest, z, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(y, highest - dist_from_each / 2.0f, z, 1.0f),
                         insideWallColor, center);

      createWallParticle(particles[counter++], glm::vec4(y, z, lowest, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(y, z, lowest + dist_from_each / 2.0f, 1.0f),
                         insideWallColor, center);

      createWallParticle(particles[counter++], glm::vec4(y, z, highest, 1.0f),
                         outsideWallColor, center);
      createWallParticle(particles[counter++],
                         glm::vec4(y, z, highest - dist_from_each / 2.0f, 1.0f),
                         insideWallColor, center);
    }
  }

  // additional particles in the corner of the cube so the fluid won't get stuck
  float corner = 0.7f;
  createWallParticle(particles[counter++],
                     glm::vec4(corner, corner, corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(corner, corner, -corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(corner, -corner, corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(corner, -corner, -corner, 1.0f), insideWallColor,
                     center);

  createWallParticle(particles[counter++],
                     glm::vec4(-corner, corner, corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(-corner, corner, -corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(-corner, -corner, corner, 1.0f), insideWallColor,
                     center);
  createWallParticle(particles[counter++],
                     glm::vec4(-corner, -corner, -corner, 1.0f),
                     insideWallColor, center);
}

void Utils::createWallParticle(Particle &particle, glm::vec4 position,
                               glm::vec4 color, glm::vec3 center) {
  particle.position = position + glm::vec4(center, 0);
  particle.velocity = glm::vec3(0.0f);
  particle.color = color;
  particle.movable = 0.0f;
  particle.acc = glm::vec3(0.0, 0.0, 0.0);
}

void Utils::createSphere(std::vector<glm::vec4> &vertices, float radius,
                         int latitudeSegments, int longitudeSegments) {
  constexpr float PI = glm::pi<float>();
  float sectorStep = 2 * PI / latitudeSegments;
  float stackStep = PI / longitudeSegments;
  float sectorAngle, stackAngle;
  std::vector<glm::vec4> sphereVertices;
  for (int i = 0; i <= longitudeSegments; ++i) {
    stackAngle = PI / 2 - i * stackStep;  // starting from pi/2 to -pi/2
    float xy = radius * cosf(stackAngle); // r * cos(u)
    float z = radius * sinf(stackAngle);  // r * sin(u)

    for (int j = 0; j <= latitudeSegments; ++j) {
      sectorAngle = j * sectorStep; // starting from 0 to 2pi

      float x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
      float y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
      sphereVertices.push_back(glm::vec4(x, y, z, 1.0f));
    }
  }

  std::vector<int> indices;
  int k1, k2;
  for (int i = 0; i < longitudeSegments; ++i) {
    k1 = i * (latitudeSegments + 1); // beginning of current stack
    k2 = k1 + latitudeSegments + 1;  // beginning of next stack

    for (int j = 0; j < latitudeSegments; ++j, ++k1, ++k2) {
      if (i != 0) {
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);
      }

      if (i != (longitudeSegments - 1)) {
        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }
    }
  }

  for (int i = 0; i < indices.size(); ++i) {
    vertices.emplace_back(sphereVertices[indices[i]]);
  }
  setVertexCount(vertices.size());
}

glm::mat4 Utils::updateCamera(float deltaTime, InputState &inputState,
                              ImGuiIO &io) {
  float movementSpeed = inputState.cameraSpeed * deltaTime;
  glm::vec3 movementDirection =
      glm::vec3(inputState.cameraFront.x, inputState.cameraFront.y, 0.0f);
  glm::vec3 movementDirectionRight =
      glm::vec3(inputState.cameraRight.x, inputState.cameraRight.y, 0.0f);
  {
    if (io.KeysDown['W']) {
      inputState.cameraPos += movementDirection * movementSpeed;
    }
    if (io.KeysDown['S']) {
      inputState.cameraPos -= movementDirection * movementSpeed;
    }
    if (io.KeysDown['D']) {
      inputState.cameraPos += movementDirectionRight * movementSpeed;
    }
    if (io.KeysDown['A']) {
      inputState.cameraPos -= movementDirectionRight * movementSpeed;
    }
    if (io.KeysDown['E']) {
      inputState.cameraPos += inputState.cameraUp * movementSpeed;
    }
    if (io.KeysDown['Q']) {
      inputState.cameraPos -= inputState.cameraUp * movementSpeed;
    }
  }
  const float rotationSpeed = 0.001f;
  if (io.MouseDown[ImGuiMouseButton_Left]) {
    if (!io.WantCaptureMouse) {
      inputState.cameraRotations.x = -1 * io.MouseDelta.y * rotationSpeed;
      inputState.cameraRotations.y = -1 * io.MouseDelta.x * rotationSpeed;
    }
  }

  glm::mat4 rotationX = glm::rotate(
      glm::mat4(1.0f), inputState.cameraRotations.x, inputState.cameraRight);
  glm::mat4 rotationY = glm::rotate(
      glm::mat4(1.0f), inputState.cameraRotations.y, inputState.cameraUp);
  glm::mat4 rotation = rotationX * rotationY;
  glm::vec3 cameraUp = inputState.cameraUp;
  inputState.cameraFront = glm::normalize(
      glm::vec3(rotation * glm::vec4(inputState.cameraFront, 1.0f)));
  inputState.cameraUp = glm::normalize(
      glm::cross(inputState.cameraRight, inputState.cameraFront));
  inputState.cameraRight =
      glm::normalize(glm::cross(inputState.cameraFront, cameraUp));
  inputState.cameraRight.z = 0.0f;
  inputState.cameraRight = glm::normalize(inputState.cameraRight);

  return glm::lookAt(inputState.cameraPos,
                     inputState.cameraPos + inputState.cameraFront,
                     inputState.cameraUp);
}