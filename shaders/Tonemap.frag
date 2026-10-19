#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(set = 0, binding = 0) uniform sampler2D hdr;

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 color = texture(hdr, inUv).rgb * frame.exposure;
  outColor = vec4(AcesTonemap(color), 1.0);
}
