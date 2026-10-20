#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  FoamRW foam;
  uint foamCapacity;
  float foamRadius;
};

layout(location = 0) in vec2 inOffset;
layout(location = 1) flat in float inLife;
layout(location = 0) out vec4 outColor;

void main() {
  if (inLife <= 0.0) {
    discard;
  }
  const float r2 = dot(inOffset, inOffset);
  if (r2 >= 1.0) {
    discard;
  }
  const float fade = smoothstep(0.0, 0.14, inLife);
  const float lobe = pow(1.0 - r2, 2.0);
  const float alpha = lobe * fade * 0.50;
  if (alpha < 0.02) {
    discard;
  }
  const vec3 wet = vec3(0.70, 0.80, 0.84);
  const vec3 lift = vec3(0.90, 0.93, 0.94);
  const vec3 color = mix(wet, lift, lobe * 0.28);
  outColor = vec4(color, alpha);
}
