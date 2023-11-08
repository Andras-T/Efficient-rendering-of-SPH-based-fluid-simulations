#version 450

struct Particle {
  vec4 position;
  vec3 velocity;
  float movable;
  vec3 acc;
  float density;
};

layout(binding = 0) uniform MVP {
  mat4 model;
  mat4 view;
  mat4 proj;
  mat4 viewModel;
  mat4 inverseModel;
  mat4 inverseProj;
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
  float farPlaneDistance;
  float particleRadius;
  vec3 pad;
}
model;

layout(location = 0) in vec4 inPosition;
layout(location = 0) out float movable;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 toCamera;
layout(location = 3) out vec3 cameraPos;

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
  Particle particlesIn[];
};

void main() {
  int instanceIndex = gl_InstanceIndex;
  Particle p = particlesIn[instanceIndex];

  gl_Position = mvp.proj *
                (vec4(inPosition.xyz, 0.0) + mvp.viewModel * p.position);
  
  movable = p.movable;
  texCoord = inPosition.xy;
  vec4 transformedP = mvp.model * p.position;
  toCamera = mvp.cameraPos - transformedP.xyz;
  cameraPos = mvp.cameraPos;
}