#version 450

layout(location = 0) in float movable;
layout(location = 0) out vec4 outColor;

layout(binding = 4) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float pad2;
}
model;

void main() {
  if (movable < 0.5) {
    // if show wall is off
    if (model.wall == 0) {
      discard;
    } else {
      outColor = model.wallColor;
    }
  } else {
    outColor = model.color;
  }
}