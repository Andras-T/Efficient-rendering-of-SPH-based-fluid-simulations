#ifndef SPH_PUSH_GLSL
#define SPH_PUSH_GLSL

layout(push_constant, scalar) uniform PushConstants {
  ParticleRW particles;
  ParticleRW sorted;
  UintRW cellOf;
  UintRW dest;
  UintRW counts;
  UintRW offsets;
  UintRW fill;
  UintRW blockSums;
  ParamsBuffer params;
} pc;

#endif
