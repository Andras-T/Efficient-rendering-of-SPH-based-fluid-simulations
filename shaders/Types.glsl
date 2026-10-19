#ifndef SPH_TYPES_GLSL
#define SPH_TYPES_GLSL

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

const uint ParticleFlagFluid = 0;
const uint ParticleFlagBoundary = 1;

struct Particle {
  vec3 position;
  float mass;
  vec3 velocity;
  float density;
  vec3 force;
  float pressure;
  uint flags;
  float pad0;
  float pad1;
  float pad2;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer ParticleBuffer {
  Particle particles[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer FrameBuffer {
  mat4 view;
  mat4 projection;
  vec3 lightDirection;
  float radius;
  ParticleBuffer particles;
  uint count;
  uint pad;
};

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

#endif
