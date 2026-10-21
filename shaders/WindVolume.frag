#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Wind.glsl"
#include "Screen.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  MacroRW macro;
  OccupancyRW occupancy;
  uvec3 grid;
  float cellSize;
  vec3 domainOrigin;
  float inletUx;
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

bool IntersectAabb(vec3 rayOrigin, vec3 rayDir, vec3 bmin, vec3 bmax, out float tEnter,
                   out float tExit) {
  const vec3 inv = 1.0 / rayDir;
  const vec3 t0 = (bmin - rayOrigin) * inv;
  const vec3 t1 = (bmax - rayOrigin) * inv;
  const vec3 tmin = min(t0, t1);
  const vec3 tmax = max(t0, t1);
  tEnter = max(max(tmin.x, tmin.y), tmin.z);
  tExit = min(min(tmax.x, tmax.y), tmax.z);
  return tExit > max(tEnter, 0.0);
}

void main() {
  const vec3 viewDir = normalize(ViewDir(inUv, frame.projection));
  const vec3 cam = frame.inverseView[3].xyz;
  const vec3 dir = normalize(mat3(frame.inverseView) * viewDir);
  const vec3 bmin = domainOrigin;
  const vec3 bmax = domainOrigin + vec3(grid) * cellSize;
  float tEnter = 0.0;
  float tExit = 0.0;
  if (!IntersectAabb(cam, dir, bmin, bmax, tEnter, tExit)) {
    discard;
  }
  tEnter = max(tEnter, 0.0);
  const int steps = 80;
  const float span = tExit - tEnter;
  const float stepLen = span / float(steps);
  vec3 emitted = vec3(0.0);
  float transmittance = 1.0;
  const vec3 freeStream = vec3(inletUx, 0.0, 0.0);
  for (int s = 0; s < steps; ++s) {
    const vec3 world = cam + dir * (tEnter + (float(s) + 0.5) * stepLen);
    const vec3 localCell = (world - domainOrigin) / cellSize;
    const ivec3 cell = ivec3(floor(localCell));
    if (any(lessThan(cell, ivec3(0))) || any(greaterThanEqual(cell, ivec3(grid)))) {
      continue;
    }
    const uint occ = occupancy.data[WindCellIndex(uvec3(cell), grid)];
    if (WindIsBlocked(occ)) {
      continue;
    }
    const vec4 sampleMacro = WindInterpMacro(macro, occupancy, world, domainOrigin, cellSize, grid);
    const vec3 vel = sampleMacro.yzw;
    const float speed = length(vel);
    const float wake = length(vel - freeStream);
    const float deficit = clamp(1.0 - speed / max(inletUx, 0.04), 0.0, 1.0);
    const float shear = wake / max(inletUx, 0.04);
    const float density = smoothstep(0.04, 0.40, max(deficit, shear * 0.75));
    if (density < 0.02) {
      continue;
    }
    float extinction = density * stepLen * 1.6;
    vec3 lightColor;
    if (frame.debugView == DebugViewVelocity) {
      lightColor = Heatmap(clamp(speed / max(inletUx * 1.6, 0.04), 0.0, 1.0));
      extinction *= 0.85;
    } else {
      lightColor = mix(vec3(0.10, 0.36, 0.78), vec3(0.62, 0.88, 1.0),
                       clamp(speed / max(inletUx, 0.04), 0.0, 1.0));
    }
    emitted += transmittance * density * stepLen * lightColor * 3.6;
    transmittance *= exp(-extinction);
    if (transmittance < 0.02) {
      break;
    }
  }
  const float alpha = 1.0 - transmittance;
  if (alpha < 0.012) {
    discard;
  }
  outColor = vec4(emitted / max(alpha, 1.0e-4), alpha);
}
