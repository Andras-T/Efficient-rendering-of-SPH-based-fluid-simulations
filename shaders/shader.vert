#version 450

struct Particle {
  vec4 position;
  vec4 color;
  vec3 velocity;
  float movable;
  vec3 acc;
  float density;
};

layout(binding = 0) uniform MVP {
  mat4 model;
  mat4 view;
  mat4 proj;
  vec3 cameraPos;
  float deltaTime;
}
mvp;

layout(binding = 4) uniform Model {
  vec4 color;
  vec4 wallColor;
  float size;
  float wallSize;
  int wall;
  float pad;
}
model;

layout(location = 0) in vec4 inPosition;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
  Particle particlesIn[];
};

void main() {
  int instanceIndex = gl_InstanceIndex;
  Particle p = particlesIn[instanceIndex];

  vec4 vertexPosition = inPosition + p.position;
  gl_Position = mvp.proj * mvp.view * mvp.model * vertexPosition;

  fragColor = p.color;
}