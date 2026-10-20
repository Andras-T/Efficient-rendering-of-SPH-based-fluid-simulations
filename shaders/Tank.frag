#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(set = 0, binding = 0) uniform sampler2D hdri;

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec3 inWorld;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

const float GlassIor = 1.52;
const float GlassF0 = pow((1.0 - GlassIor) / (1.0 + GlassIor), 2.0);
const float PaneMeters = 0.012;

void main() {
  const vec3 eye = frame.inverseView[3].xyz;
  vec3 normal = normalize(inNormal);
  const vec3 view = normalize(eye - inWorld);
  if (dot(normal, view) < 0.0) {
    normal = -normal;
  }

  const float facing = clamp(dot(normal, view), 0.0, 1.0);
  const float fresnel = GlassF0 + (1.0 - GlassF0) * pow(1.0 - facing, 5.0);
  const vec3 reflected = reflect(-view, normal);
  vec3 refracted = refract(-view, normal, 1.0 / GlassIor);
  if (refracted == vec3(0.0)) {
    refracted = reflected;
  }
  const vec3 envRefl = SampleEquirect(hdri, reflected);
  const vec3 envRefr = SampleEquirect(hdri, refracted);
  const float path = PaneMeters / max(facing, 0.08);
  const vec3 tint = exp(-vec3(1.35, 0.55, 0.28) * path);
  vec3 color = mix(envRefr * tint * vec3(0.82, 0.93, 0.98), envRefl, fresnel);

  const vec3 extent = max(frame.domainMax - frame.domainMin, vec3(1.0e-4));
  const vec3 t = (inWorld - frame.domainMin) / extent;
  const vec3 faceDist = min(t, 1.0 - t);
  const float nearest = min(faceDist.x, min(faceDist.y, faceDist.z));
  const float farthest = max(faceDist.x, max(faceDist.y, faceDist.z));
  const float mid = faceDist.x + faceDist.y + faceDist.z - nearest - farthest;
  const float frameMask = 1.0 - smoothstep(0.010, 0.032, mid);
  const vec3 light = normalize(frame.lightDirection);
  const float rim = pow(1.0 - facing, 2.0);
  const float spec = pow(max(dot(normal, normalize(light + view)), 0.0), 64.0);
  const vec3 frameAlbedo = vec3(0.55, 0.62, 0.68);
  color = mix(color, frameAlbedo * (0.18 + 0.55 * spec) + envRefl * (0.22 + 0.45 * rim), frameMask);

  const float alpha = mix(mix(0.10, 0.58, fresnel), mix(0.42, 0.82, spec + rim * 0.35), frameMask);
  outColor = vec4(color, alpha);
}
