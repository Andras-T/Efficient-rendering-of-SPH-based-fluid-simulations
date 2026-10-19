#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  FoamRW foam;
  uint foamCapacity;
  float foamRadius;
};

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec3 inViewCenter;
layout(location = 2) flat in float inLife;
layout(location = 0) out vec4 outColor;

void main() {
  if (inLife <= 0.0) {
    discard;
  }
  const vec3 ray = normalize(inViewPos);
  const vec3 center = inViewCenter;
  const float radius = foamRadius * mix(0.45, 1.0, clamp(inLife, 0.0, 1.0));
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
  const vec3 hit = ray * t;
  const vec4 clip = frame.projection * vec4(hit, 1.0);
  gl_FragDepth = clip.z / clip.w;
  const float glow = clamp(inLife, 0.0, 1.0);
  outColor = vec4(vec3(0.92, 0.96, 1.0) * (0.18 * glow), 0.0);
}
