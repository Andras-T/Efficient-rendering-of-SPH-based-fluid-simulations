#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) out vec3 outViewPos;
layout(location = 1) out vec3 outViewCenter;
layout(location = 2) flat out uint outFlags;
layout(location = 3) flat out float outDensity;
layout(location = 4) flat out float outSpeed;
layout(location = 5) flat out uint outCell;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

uint CellOf(vec3 position) {
  const vec3 local = (position - frame.domainMin) * frame.invCellSize;
  const ivec3 cell = clamp(ivec3(floor(local)), ivec3(0), ivec3(frame.gridSize) - 1);
  return uint(cell.x) + uint(cell.y) * frame.gridSize.x +
         uint(cell.z) * frame.gridSize.x * frame.gridSize.y;
}

void main() {
  const Particle particle = frame.particles.particles[gl_InstanceIndex];
  const vec3 center = (frame.view * vec4(particle.position, 1.0)).xyz;
  const vec2 corner = kCorners[gl_VertexIndex];
  const vec3 viewPos = center + vec3(corner * (frame.radius * 2.0), 0.0);
  outViewPos = viewPos;
  outViewCenter = center;
  outFlags = particle.flags;
  outDensity = particle.density;
  outSpeed = length(particle.velocity);
  outCell = CellOf(particle.position);
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
