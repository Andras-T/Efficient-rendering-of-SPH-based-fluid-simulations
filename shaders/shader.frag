#version 450

layout(location = 0) in float movable;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 toCamera;

layout(location = 0) out vec4 outColor;

layout(binding = 4) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float farPlaneDistance;
}
model;

void main() {
  float distance = length(texCoord);
  float radius = 0.075f;
  if (distance > radius)
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

  gl_FragDepth =
      (length(toCamera) - sqrt(radius * radius - distance * distance)) /
      model.farPlaneDistance;
}