#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  vec3 bmin;
  vec3 bmax;
};

layout(location = 0) in vec3 inWorld;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 normal = normalize(inNormal);
  const vec3 light = normalize(frame.lightDirection);
  const float diffuse = max(dot(normal, light), 0.0);
  const vec3 albedo = mix(vec3(0.66, 0.40, 0.18), vec3(0.42, 0.24, 0.10),
                          fract(inWorld.y * 7.0 + inWorld.x * 0.5));
  const vec3 color = albedo * (0.22 + 0.78 * diffuse);
  outColor = vec4(color, 1.0);
}
