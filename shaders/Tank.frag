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

void main() {
  const vec3 eye = frame.inverseView[3].xyz;
  const vec3 normal = normalize(inNormal);
  const vec3 view = normalize(eye - inWorld);
  const float facing = abs(dot(normal, view));
  const float fresnel = mix(0.05, 0.62, pow(1.0 - clamp(facing, 0.0, 1.0), 5.0));
  const vec3 reflected = reflect(-view, normal);
  const vec3 env = SampleEquirect(hdri, reflected);
  const vec3 tint = vec3(0.70, 0.86, 0.94);
  const vec3 color = mix(tint * 0.18, env, fresnel);
  const float alpha = mix(0.07, 0.50, fresnel) + (inNormal.y < -0.5 ? 0.08 : 0.0);
  outColor = vec4(color, alpha);
}
