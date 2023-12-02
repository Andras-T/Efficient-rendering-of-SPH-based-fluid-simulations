#version 450

layout(location = 0) in float movable;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 toCamera;
layout(location = 3) in vec3 cameraPos;
layout(location = 4) in vec4 particlePos;

layout(location = 0) out vec4 outColor;

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
  vec2 windowSize;
  int wall;
  float farPlaneDistance;
  float particleRadius;
  vec3 pad;
}
model;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

void main() {
  float distance = length(texCoord);
  float radius = model.particleRadius;
  if (constants.stageIndex == 0) {
    radius *= 1.2f;
  }
  if (distance > radius )
    discard;

  if (movable < 0.5f) {
    if (model.wall == 0) {
      discard;
    } else {
      outColor = model.wallColor;
    }
  } else {
    outColor = model.color;
  }
  
    vec4 pos = mvp.proj * (vec4(texCoord, sqrt(radius * radius - distance * distance), 0.0) + mvp.viewModel * particlePos);
    gl_FragDepth = pos.z/pos.w;
}