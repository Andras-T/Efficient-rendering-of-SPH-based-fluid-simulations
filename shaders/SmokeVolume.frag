#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Smoke.glsl"
#include "Screen.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  ScalarRW scalar;
  VelocityRW velocity;
  uvec3 grid;
  float cellSize;
  vec3 domainOrigin;
  float pad;
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
  const int steps = 72;
  const float span = tExit - tEnter;
  const float stepLen = span / float(steps);
  vec3 emitted = vec3(0.0);
  float transmittance = 1.0;
  const vec3 light = normalize(frame.lightDirection);
  for (int s = 0; s < steps; ++s) {
    const vec3 world = cam + dir * (tEnter + (float(s) + 0.5) * stepLen);
    const vec2 field = SmokeInterpScalar(scalar, world, domainOrigin, cellSize, grid);
    const float density = field.x;
    if (density < 0.01) {
      continue;
    }
    const vec3 vel = SmokeInterpVelocity(velocity, world, domainOrigin, cellSize, grid);
    float extinction = density * stepLen * 2.6;
    vec3 lightColor;
    if (frame.debugView == DebugViewVelocity) {
      lightColor = Heatmap(clamp(length(vel) / 1.15, 0.0, 1.0));
      extinction *= 0.75;
    } else {
      const vec3 albedo = mix(vec3(0.78, 0.80, 0.84), vec3(1.0, 0.62, 0.28), clamp(field.y, 0.0, 1.0));
      const vec3 dx = vec3(cellSize, 0.0, 0.0);
      const vec3 dy = vec3(0.0, cellSize, 0.0);
      const vec3 dz = vec3(0.0, 0.0, cellSize);
      const vec3 gradient =
          vec3(SmokeInterpScalar(scalar, world + dx, domainOrigin, cellSize, grid).x -
                   SmokeInterpScalar(scalar, world - dx, domainOrigin, cellSize, grid).x,
               SmokeInterpScalar(scalar, world + dy, domainOrigin, cellSize, grid).x -
                   SmokeInterpScalar(scalar, world - dy, domainOrigin, cellSize, grid).x,
               SmokeInterpScalar(scalar, world + dz, domainOrigin, cellSize, grid).x -
                   SmokeInterpScalar(scalar, world - dz, domainOrigin, cellSize, grid).x);
      const vec3 normal = length(gradient) > 1.0e-5 ? normalize(-gradient) : vec3(0.0, 1.0, 0.0);
      const float lambert = 0.38 + 0.62 * max(dot(normal, light), 0.0);
      lightColor = albedo * lambert;
    }
    emitted += transmittance * density * stepLen * lightColor * 2.4;
    transmittance *= exp(-extinction);
    if (transmittance < 0.01) {
      break;
    }
  }
  const float alpha = 1.0 - transmittance;
  if (alpha < 0.008) {
    discard;
  }
  outColor = vec4(emitted / max(alpha, 1.0e-4), alpha);
}
