#ifndef SPH_KERNELS_GLSL
#define SPH_KERNELS_GLSL

const float Pi = 3.14159265358979323846;
const float KernelEpsilon = 1.0e-8;

float Poly6(float distanceSquared, float kernelRadius) {
  const float h2 = kernelRadius * kernelRadius;
  if (distanceSquared >= h2) {
    return 0.0;
  }
  const float scale = 315.0 / (64.0 * Pi * pow(kernelRadius, 9.0));
  const float delta = h2 - distanceSquared;
  return scale * delta * delta * delta;
}

vec3 SpikyGradient(vec3 offset, float kernelRadius) {
  const float distance = length(offset);
  if (distance <= KernelEpsilon || distance >= kernelRadius) {
    return vec3(0.0);
  }
  const float scale = -45.0 / (Pi * pow(kernelRadius, 6.0));
  const float gap = kernelRadius - distance;
  return offset * ((scale * gap * gap) / distance);
}

float ViscosityLaplacian(float distance, float kernelRadius) {
  if (distance >= kernelRadius) {
    return 0.0;
  }
  return (45.0 / (Pi * pow(kernelRadius, 6.0))) * (kernelRadius - distance);
}

float TaitPressure(float density, float restDensity, float stiffness, float gamma) {
  const float ratio = max(density, KernelEpsilon) / restDensity;
  return max(0.0, stiffness * (pow(ratio, gamma) - 1.0));
}

ivec3 CellCoords(vec3 position, GpuSimParams p) {
  const vec3 local = (position - p.domainMin) * p.invCellSize;
  ivec3 cell = ivec3(floor(local));
  const ivec3 last = ivec3(p.gridSize) - ivec3(1);
  return clamp(cell, ivec3(0), last);
}

uint CellIndex(ivec3 cell, GpuSimParams p) {
  return uint(cell.x) + uint(cell.y) * p.gridSize.x + uint(cell.z) * p.gridSize.x * p.gridSize.y;
}

#endif
