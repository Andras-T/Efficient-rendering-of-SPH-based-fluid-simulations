#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec3 inViewCenter;
layout(location = 2) flat in uint inFlags;
layout(location = 3) flat in float inRadius;
layout(location = 0) out float outThickness;

void main() {
  if (inFlags == ParticleFlagBoundary || inRadius <= 0.0) {
    discard;
  }
  const vec3 ray = normalize(inViewPos);
  const vec3 center = inViewCenter;
  const float radius = inRadius;
  const float b = dot(center, ray);
  const float c = dot(center, center) - radius * radius;
  const float disc = b * b - c;
  if (disc < 0.0) {
    discard;
  }
  const float t = b - sqrt(disc);
  if (t < 0.0) {
    discard;
  }
  outThickness = 2.0 * sqrt(disc);
}
