#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  FoamRW foam;
  uint foamCapacity;
  float foamRadius;
};

layout(location = 0) out vec3 outViewPos;
layout(location = 1) out vec3 outViewCenter;
layout(location = 2) flat out float outLife;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  const FoamParticle particle = foam.particles[gl_InstanceIndex];
  if (particle.alive == 0u || particle.life <= 0.0) {
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    outLife = 0.0;
    outViewPos = vec3(0.0);
    outViewCenter = vec3(0.0);
    return;
  }

  const vec3 center = (frame.view * vec4(particle.position, 1.0)).xyz;
  const vec2 corner = kCorners[gl_VertexIndex];
  const float radius = foamRadius * mix(0.45, 1.0, clamp(particle.life, 0.0, 1.0));
  const vec3 viewPos = center + vec3(corner * (radius * 2.0), 0.0);
  outViewPos = viewPos;
  outViewCenter = center;
  outLife = particle.life;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
