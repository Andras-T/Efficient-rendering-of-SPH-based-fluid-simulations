#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) out vec2 coord;

void main() {
  coord = (inPosition.xy + vec2(1.0f)) * 0.5f;
  gl_Position = inPosition;
}