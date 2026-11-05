#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) out vec2 outCorner;
layout(location = 1) flat out float outSpeed;
layout(location = 2) flat out float outVorticity;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  const Particle particle = frame.particles.particles[gl_InstanceIndex];
  const vec3 head = particle.position;
  vec3 tail = particle.force;
  vec3 delta = head - tail;
  if (dot(delta, delta) < 1.0e-8) {
    delta = particle.velocity;
  }
  if (dot(delta, delta) < 1.0e-8) {
    delta = vec3(-0.08, 0.0, 0.0);
  }
  const float span = clamp(length(delta), 0.055, 0.26);
  tail = head - normalize(delta) * span;

  const vec3 vHead = (frame.view * vec4(head, 1.0)).xyz;
  const vec3 vTail = (frame.view * vec4(tail, 1.0)).xyz;
  vec2 along = vHead.xy - vTail.xy;
  vec2 n = vec2(-along.y, along.x);
  const float nLen = length(n);
  n = nLen > 1.0e-8 ? n / nLen : vec2(1.0, 0.0);
  n *= frame.radius;

  const vec2 corner = kCorners[gl_VertexIndex];
  const vec3 viewPos = mix(vTail, vHead, corner.x * 0.5 + 0.5) + vec3(n * corner.y, 0.0);
  outCorner = corner;
  outSpeed = length(particle.velocity);
  outVorticity = particle.kappa;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
