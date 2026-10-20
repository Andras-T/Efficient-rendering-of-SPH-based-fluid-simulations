#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec3 inViewCenter;
layout(location = 2) flat in float inSpeed;
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 ray = normalize(inViewPos);
  const vec3 center = inViewCenter;
  const float radius = frame.radius;
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

  if (frame.debugView == DebugViewVelocity) {
    outColor = vec4(Heatmap(clamp(inSpeed / 0.12, 0.0, 1.0)), 1.0);
    return;
  }
  const vec3 normal = normalize(hit - center);
  const float rim = pow(1.0 - abs(dot(normal, -ray)), 2.0);
  const vec3 smoke = mix(vec3(0.82, 0.86, 0.90), vec3(0.55, 0.62, 0.70), rim);
  outColor = vec4(smoke, 0.22 + 0.20 * rim);
}
