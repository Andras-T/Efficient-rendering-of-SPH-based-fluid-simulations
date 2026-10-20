#ifndef SPH_TYPES_GLSL
#define SPH_TYPES_GLSL

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

const uint ParticleFlagFluid = 0;
const uint ParticleFlagBoundary = 1;
const uint SolverModeWcsph = 0;
const uint SolverModeDfsph = 1;
const uint FoamSpray = 0;
const uint FoamFoam = 1;
const uint FoamBubble = 2;
const uint SolvePassDensity = 0;
const uint SolvePassDivergence = 1;

struct Particle {
  vec3 position;
  float mass;
  vec3 velocity;
  float density;
  vec3 force;
  float pressure;
  uint flags;
  float alpha;
  float kappa;
  float divergence;
};

struct FoamParticle {
  vec3 position;
  float life;
  vec3 velocity;
  uint alive;
};

struct GpuSimParams {
  vec3 domainMin;
  float kernelRadius;
  vec3 domainMax;
  float kernelRadius2;
  vec3 gravity;
  float mass;
  float restDensity;
  float stiffness;
  float eosGamma;
  float viscosity;
  float dt;
  float restitution;
  float collisionRadius;
  float invCellSize;
  uvec3 gridSize;
  uint particleCount;
  uint cellCount;
  float cursorRadius;
  vec3 cursorPosition;
  vec3 cursorForce;
  uint solverMode;
  uint densityIterations;
  uint foamCapacity;
  float foamDt;
  float xsph;
  float cursorSwirl;
  float cursorPush;
  vec3 blockMin;
  float blockEnabled;
  vec3 blockMax;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer ParticleBuffer {
  Particle particles[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) buffer ParticleRW {
  Particle particles[];
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer UintRW {
  coherent uint data[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer ParamsBuffer {
  GpuSimParams p;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) buffer FoamRW {
  coherent FoamParticle particles[];
};

const uint DebugViewFinal = 0;
const uint DebugViewParticles = 1;
const uint DebugViewDensity = 2;
const uint DebugViewVelocity = 3;
const uint DebugViewCell = 4;
const uint DebugViewRawDepth = 5;
const uint DebugViewSmoothDepth = 6;
const uint DebugViewThickness = 7;
const uint DebugViewNormals = 8;
const uint DebugViewFoam = 9;
const uint WallHidden = 0;
const uint WallGlass = 1;
const uint WallParticles = 2;

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer FrameBuffer {
  mat4 view;
  mat4 projection;
  mat4 inverseView;
  mat4 inverseProjection;
  vec3 lightDirection;
  float radius;
  ParticleBuffer particles;
  uint count;
  uint debugView;
  vec3 absorption;
  float ior;
  vec3 domainMin;
  float restDensity;
  vec3 domainMax;
  float invCellSize;
  uvec3 gridSize;
  float exposure;
  uint wallDisplay;
};

#endif
