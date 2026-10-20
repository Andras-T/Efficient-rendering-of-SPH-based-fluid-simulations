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

void LimitSpeed(inout vec3 velocity, GpuSimParams p) {
  const float vmax = 0.40 * p.kernelRadius / max(p.dt, KernelEpsilon);
  const float speed = length(velocity);
  if (speed > vmax) {
    velocity *= vmax / speed;
  }
}

void CollideBox(inout vec3 position, inout vec3 velocity, GpuSimParams p) {
  const vec3 minBound = p.domainMin + vec3(p.collisionRadius);
  const vec3 maxBound = p.domainMax - vec3(p.collisionRadius);
  if (any(isnan(position)) || any(isinf(position)) || any(isnan(velocity)) || any(isinf(velocity))) {
    position = clamp((p.domainMin + p.domainMax) * 0.5, minBound, maxBound);
    velocity = vec3(0.0);
    return;
  }
  for (int axis = 0; axis < 3; ++axis) {
    if (position[axis] < minBound[axis]) {
      position[axis] = minBound[axis];
      if (velocity[axis] < 0.0) {
        velocity[axis] *= -p.restitution;
      }
    } else if (position[axis] > maxBound[axis]) {
      position[axis] = maxBound[axis];
      if (velocity[axis] > 0.0) {
        velocity[axis] *= -p.restitution;
      }
    }
  }
  LimitSpeed(velocity, p);
}

// `particle.force` is already SPH acceleration (m/s^2), not a Newton force.
vec3 NonPressureAccel(Particle particle, GpuSimParams p) {
  vec3 acceleration = particle.force + p.gravity;
  if (p.cursorRadius > 0.0) {
    const vec3 toParticle = particle.position - p.cursorPosition;
    const float dist2 = dot(toParticle, toParticle);
    const float radius2 = p.cursorRadius * p.cursorRadius;
    if (dist2 < radius2) {
      const float falloff = 1.0 - dist2 / radius2;
      acceleration += p.cursorForce * (falloff * falloff);
    }
  }
  return acceleration;
}

#endif
