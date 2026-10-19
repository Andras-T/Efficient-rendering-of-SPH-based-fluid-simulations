#version 460

layout(push_constant) uniform Constants {
  vec4 topColor;
  vec4 bottomColor;
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main() {
  outColor = mix(topColor, bottomColor, inUv.y);
}
