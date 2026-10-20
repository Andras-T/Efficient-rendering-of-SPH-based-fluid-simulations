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
  const vec3 light = normalize(frame.lightDirection);
  const float diffuse = max(dot(normalize(inNormal), light), 0.0);
  const vec2 stripe = floor(inWorld.xz * 2.4);
  const float check = mod(stripe.x + stripe.y, 2.0);
  const vec3 albedo = mix(vec3(0.16, 0.16, 0.17), vec3(0.11, 0.11, 0.12), check);
  outColor = vec4(albedo * (0.22 + 0.78 * diffuse), 1.0);
}
