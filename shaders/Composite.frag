#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(set = 0, binding = 0) uniform sampler2D hdri;
layout(set = 0, binding = 1) uniform sampler2D rawDepth;
layout(set = 0, binding = 2) uniform sampler2D smoothDepth;
layout(set = 0, binding = 3) uniform sampler2D thicknessMap;

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

float SampleDepth(sampler2D depthImage, vec2 uv) {
  return texture(depthImage, uv).r;
}

vec3 FluidNormal(vec2 uv, float center, vec2 texel) {
  const vec2 tap = texel * 1.25;
  float zx1 = SampleDepth(smoothDepth, uv + vec2(tap.x, 0.0));
  float zx0 = SampleDepth(smoothDepth, uv - vec2(tap.x, 0.0));
  float zy1 = SampleDepth(smoothDepth, uv + vec2(0.0, tap.y));
  float zy0 = SampleDepth(smoothDepth, uv - vec2(0.0, tap.y));

  vec2 offset = vec2(tap.x, 0.0);
  float neighbor = zx1;
  if (zx1 <= 0.0 || (zx0 > 0.0 && abs(zx0 - center) < abs(zx1 - center))) {
    offset = vec2(-tap.x, 0.0);
    neighbor = zx0;
  }
  const vec3 p = ViewPosition(uv, center, frame.projection);
  vec3 dx = ViewPosition(uv + offset, neighbor > 0.0 ? neighbor : center, frame.projection) - p;

  offset = vec2(0.0, tap.y);
  neighbor = zy1;
  if (zy1 <= 0.0 || (zy0 > 0.0 && abs(zy0 - center) < abs(zy1 - center))) {
    offset = vec2(0.0, -tap.y);
    neighbor = zy0;
  }
  vec3 dy = ViewPosition(uv + offset, neighbor > 0.0 ? neighbor : center, frame.projection) - p;

  vec3 normal = normalize(cross(dx, dy));
  if (dot(normal, -p) < 0.0) {
    normal = -normal;
  }
  return normal;
}

vec3 ShadeFluid(vec2 uv, float eyeDepth, vec3 normal, float thickness) {
  const vec3 viewPos = ViewPosition(uv, eyeDepth, frame.projection);
  const vec3 viewDir = normalize(-viewPos);
  vec3 worldN = normalize(mat3(frame.inverseView) * normal);
  const vec3 worldP = (frame.inverseView * vec4(viewPos, 1.0)).xyz;
  const vec3 worldI = normalize(mat3(frame.inverseView) * (-viewDir));

  const vec3 gap = min(worldP - frame.domainMin, frame.domainMax - worldP);
  const float wall = min(gap.x, min(gap.y, gap.z));
  const float contact = 1.0 - smoothstep(0.0, frame.radius * 2.6, wall);
  worldN = normalize(mix(worldN, vec3(0.0, 1.0, 0.0), contact * 0.18));

  const vec3 reflected = reflect(worldI, worldN);
  vec3 refracted = refract(worldI, worldN, 1.0 / max(frame.ior, 1.01));
  if (refracted == vec3(0.0)) {
    refracted = reflected;
  }
  const vec3 envRefl = SampleEquirect(hdri, reflected);
  const vec3 envRefr = SampleEquirect(hdri, refracted);
  const float optical = max(thickness, 0.0) * 2.85;
  vec3 transmitted = envRefr * exp(-frame.absorption * optical);
  transmitted *= mix(1.0, 0.70, contact);

  const float ndotv = clamp(dot(normal, viewDir), 0.0, 1.0);
  const float f0 = pow((1.0 - frame.ior) / (1.0 + frame.ior), 2.0);
  float fresnel = f0 + (1.0 - f0) * pow(1.0 - ndotv, 5.0);
  fresnel = mix(fresnel, min(1.0, fresnel + 0.22), contact);
  vec3 color = mix(transmitted, envRefl, fresnel);
  color += envRefl * envRefl * (0.04 + 0.10 * fresnel);
  return color;
}

void main() {
  const vec2 texel = 1.0 / vec2(textureSize(smoothDepth, 0));
  const float eyeDepth = (frame.debugView == DebugViewRawDepth) ? SampleDepth(rawDepth, inUv)
                                                               : SampleDepth(smoothDepth, inUv);
  const float thickness = texture(thicknessMap, inUv).r;

  if (frame.debugView == DebugViewThickness) {
    outColor = vec4(Heatmap(thickness * 1.6), 1.0);
    return;
  }
  if (eyeDepth <= 0.0 ||
      (frame.debugView == DebugViewFinal && thickness < frame.radius * 0.45)) {
    if (frame.debugView == DebugViewFinal) {
      discard;
    }
    outColor = vec4(0.04, 0.05, 0.07, 1.0);
    return;
  }
  if (frame.debugView == DebugViewRawDepth || frame.debugView == DebugViewSmoothDepth) {
    outColor = vec4(Heatmap(1.0 - exp(-eyeDepth * 0.55)), 1.0);
    return;
  }

  const vec3 normal = FluidNormal(inUv, SampleDepth(smoothDepth, inUv), texel);
  if (frame.debugView == DebugViewNormals) {
    outColor = vec4(normal * 0.5 + 0.5, 1.0);
    return;
  }

  const vec3 color = ShadeFluid(inUv, eyeDepth, normal, thickness);
  const float coverage = mix(0.78, 1.0, smoothstep(frame.radius * 0.45, frame.radius * 1.8, thickness));
  const float alpha = (frame.debugView == DebugViewFinal) ? coverage : 1.0;
  outColor = vec4(color, alpha);
}
