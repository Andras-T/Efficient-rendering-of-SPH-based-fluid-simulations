#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) out vec3 outViewPos;
layout(location = 1) out vec3 outViewCenter;
layout(location = 2) flat out uint outFlags;
layout(location = 3) flat out float outRadius;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  const Particle particle = frame.particles.particles[gl_InstanceIndex];
  const float packed = max(frame.restDensity, 1.0);
  const float coverage = clamp(particle.density / packed, 0.0, 1.0);
  const float scale = smoothstep(0.18, 0.38, coverage);
  if (particle.flags == ParticleFlagFluid && scale < 0.05) {
    outViewPos = vec3(0.0);
    outViewCenter = vec3(0.0);
    outFlags = particle.flags;
    outRadius = 0.0;
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    return;
  }

  const float radius = frame.radius * (particle.flags == ParticleFlagFluid ? mix(0.70, 1.0, scale)
                                                                          : 1.0);
  const vec3 center = (frame.view * vec4(particle.position, 1.0)).xyz;
  const vec2 corner = kCorners[gl_VertexIndex];
  const vec3 viewPos = center + vec3(corner * (radius * 2.0), 0.0);
  outViewPos = viewPos;
  outViewCenter = center;
  outFlags = particle.flags;
  outRadius = radius;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
