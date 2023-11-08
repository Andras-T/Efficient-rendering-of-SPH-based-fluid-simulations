#version 450


layout(location = 0) in float movable;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 toCamera;
layout(location = 3) in vec3 cameraPos;

layout(location = 0) out vec4 outColor;

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
  if (distance > radius && constants.stageIndex == 0)
    discard;

  if (distance > 0.11 && constants.stageIndex == 1) {
    discard;
  }

  if (movable < 0.5f) {
    if (model.wall == 0) {
      discard;
    } else {
      outColor = model.wallColor;
    }
  } else {
    outColor = model.color;
  }

  if (constants.stageIndex == 0) {
  gl_FragDepth =
      (length(toCamera) - length(cameraPos) + length(vec3(1.0f)) - sqrt(radius * radius - distance * distance)) /
      model.farPlaneDistance;
  } else {
  radius = 0.11f;
  gl_FragDepth = 1.0f -
      (length(toCamera) - length(cameraPos) + length(vec3(1.0f)) - sqrt(radius * radius - distance * distance)) /
      model.farPlaneDistance;
  }
}