#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  FoamRW foam;
  uint foamCapacity;
  float foamRadius;
};

layout(location = 0) out vec2 outOffset;
layout(location = 1) flat out float outLife;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  const FoamParticle particle = foam.particles[gl_InstanceIndex];
  if (particle.alive == 0u || particle.life <= 0.0) {
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    outLife = 0.0;
    outOffset = vec2(0.0);
    return;
  }

  const float fade = smoothstep(0.0, 0.18, particle.life);
  const float radius = foamRadius * mix(0.80, 1.20, fade);
  vec3 center = (frame.view * vec4(particle.position, 1.0)).xyz;
  center.z += radius * 0.40;
  const vec2 corner = kCorners[gl_VertexIndex];
  const vec3 viewPos = center + vec3(corner * radius, 0.0);
  outOffset = corner;
  outLife = particle.life;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
