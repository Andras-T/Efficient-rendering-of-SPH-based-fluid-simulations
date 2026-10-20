#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec3 inViewCenter;
layout(location = 2) flat in uint inFlags;
layout(location = 3) flat in float inDensity;
layout(location = 4) flat in float inSpeed;
layout(location = 5) flat in uint inCell;
layout(location = 0) out vec4 outColor;

vec3 CellColor(uint cell) {
  const uint hash = cell * 747796405u + 2891336453u;
  return vec3(float(hash & 255u), float((hash >> 8u) & 255u), float((hash >> 16u) & 255u)) / 255.0;
}

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
  const vec3 normal = normalize(hit - center);
  const vec4 clip = frame.projection * vec4(hit, 1.0);
  gl_FragDepth = clip.z / clip.w;

  if (inFlags == ParticleFlagBoundary && frame.wallDisplay != WallParticles) {
    discard;
  }
  if (frame.debugView == DebugViewFinal && inFlags != ParticleFlagBoundary) {
    discard;
  }

  if (frame.debugView == DebugViewDensity) {
    outColor = vec4(Heatmap(inDensity / max(frame.restDensity, 1.0)), 1.0);
    return;
  }
  if (frame.debugView == DebugViewVelocity) {
    outColor = vec4(Heatmap(clamp(inSpeed * 0.45, 0.0, 1.0)), 1.0);
    return;
  }
  if (frame.debugView == DebugViewCell) {
    outColor = vec4(CellColor(inCell), 1.0);
    return;
  }

  const vec3 light = normalize(mat3(frame.view) * frame.lightDirection);
  const float diffuse = max(dot(normal, light), 0.0);
  const vec3 viewDir = normalize(-hit);
  const vec3 halfDir = normalize(light + viewDir);
  const float specular = pow(max(dot(normal, halfDir), 0.0), 32.0);
  const vec3 albedo = inFlags == ParticleFlagBoundary ? vec3(0.42, 0.40, 0.38)
                                                      : vec3(0.12, 0.42, 0.82);
  const vec3 color = albedo * (0.16 + 0.84 * diffuse) + vec3(0.40 * specular);
  outColor = vec4(color, 1.0);
}
