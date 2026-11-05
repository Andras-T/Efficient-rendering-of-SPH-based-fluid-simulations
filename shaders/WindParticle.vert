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
  const vec3 center = (frame.view * vec4(particle.position, 1.0)).xyz;
  vec2 along = (mat3(frame.view) * particle.velocity).xy;
  if (dot(along, along) < 1.0e-10) {
    along = vec2(1.0, 0.0);
  } else {
    along = normalize(along);
  }
  const vec2 n = vec2(-along.y, along.x);
  const vec2 corner = kCorners[gl_VertexIndex];
  const float halfW = frame.radius;
  const float halfL = frame.radius * 2.6;
  const vec3 viewPos = center + vec3(along * corner.x * halfL + n * corner.y * halfW, 0.0);
  outCorner = corner;
  outSpeed = length(particle.velocity);
  outVorticity = particle.kappa;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
