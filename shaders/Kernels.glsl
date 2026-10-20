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

const float DfsphKappaMax = 12.0;
const float DfsphOmega = 0.35;

float DfsphAlphaFromDenom(float denom, float dt) {
  const float alpha = 1.0 / max(1.0e-6, denom);
  const float alphaMax = 0.02 * dt * dt;
  return min(alpha, alphaMax);
}

float DfsphDensityKappa(float predicted, float restDensity, float dt, float alpha) {
  return clamp(((predicted - restDensity) / (dt * dt)) * alpha, 0.0, DfsphKappaMax);
}

float DfsphDivergenceKappa(float source, float density, float restDensity, float dt, float alpha) {
  if (density < 0.85 * restDensity) {
    return 0.0;
  }
  return clamp((source / dt) * alpha, -DfsphKappaMax, DfsphKappaMax);
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
  const float vmax = 0.15 * p.kernelRadius / max(p.dt, KernelEpsilon);
  const float speed = length(velocity);
  if (speed > vmax) {
    velocity *= vmax / speed;
  }
}

void CollideAabb(inout vec3 position, inout vec3 velocity, vec3 boxMin, vec3 boxMax, float radius,
                 float restitution) {
  const vec3 lo = boxMin - vec3(radius);
  const vec3 hi = boxMax + vec3(radius);
  if (any(lessThanEqual(position, lo)) || any(greaterThanEqual(position, hi))) {
    return;
  }
  const vec3 dLo = position - lo;
  const vec3 dHi = hi - position;
  int axis = 0;
  float push = dLo.x;
  float dir = -1.0;
  if (dHi.x < push) {
    push = dHi.x;
    dir = 1.0;
  }
  if (dLo.y < push) {
    axis = 1;
    push = dLo.y;
    dir = -1.0;
  }
  if (dHi.y < push) {
    axis = 1;
    push = dHi.y;
    dir = 1.0;
  }
  if (dLo.z < push) {
    axis = 2;
    push = dLo.z;
    dir = -1.0;
  }
  if (dHi.z < push) {
    axis = 2;
    push = dHi.z;
    dir = 1.0;
  }
  position[axis] += dir * push;
  if (velocity[axis] * dir < 0.0) {
    velocity[axis] *= -restitution;
  }
}

void CollidePaddle(inout vec3 position, inout vec3 velocity, GpuSimParams p) {
  if (p.cursorRadius <= 0.0) {
    return;
  }
  const vec3 delta = position - p.cursorPosition;
  const float dist = length(delta);
  const float minDist = p.cursorRadius + p.collisionRadius * 0.25;
  if (dist >= minDist || dist <= KernelEpsilon) {
    return;
  }
  const vec3 normal = delta / dist;
  position = p.cursorPosition + normal * minDist;
  const float into = dot(velocity, normal);
  if (into < 0.0) {
    velocity -= (1.0 + p.restitution) * into * normal;
  }
}

void CollideDomain(inout vec3 position, inout vec3 velocity, vec3 minBound, vec3 maxBound,
                   float restitution) {
  for (int axis = 0; axis < 3; ++axis) {
    if (position[axis] < minBound[axis]) {
      position[axis] = minBound[axis];
      if (velocity[axis] < 0.0) {
        velocity[axis] *= -restitution;
      }
    } else if (position[axis] > maxBound[axis]) {
      position[axis] = maxBound[axis];
      if (velocity[axis] > 0.0) {
        velocity[axis] *= -restitution;
      }
    }
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
  CollidePaddle(position, velocity, p);
  if (p.blockEnabled > 0.5) {
    CollideAabb(position, velocity, p.blockMin, p.blockMax, p.collisionRadius, p.restitution);
  }
  CollideDomain(position, velocity, minBound, maxBound, p.restitution);
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
      const float weight = falloff * falloff;
      acceleration += p.cursorForce * weight;
      if (p.cursorPush != 0.0) {
        const float dist = sqrt(max(dist2, KernelEpsilon));
        acceleration += (toParticle / dist) * p.cursorPush * weight;
      }
      if (p.cursorSwirl != 0.0) {
        const vec3 tangent = cross(vec3(0.0, 1.0, 0.0), toParticle);
        const float tangentLength = length(tangent);
        if (tangentLength > KernelEpsilon) {
          acceleration += (tangent / tangentLength) * p.cursorSwirl * weight;
        }
      }
    }
  }
  return acceleration;
}

#endif
