#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec2 inCorner;
layout(location = 1) flat in float inSpeed;
layout(location = 2) flat in float inVorticity;
layout(location = 0) out vec4 outColor;

void main() {
  const float across = inCorner.y;
  const float along = inCorner.x;
  const float envelope = smoothstep(-1.0, -0.55, along) * (1.0 - smoothstep(0.55, 1.0, along));
  const float alpha = exp(-4.2 * across * across) * envelope;
  if (alpha < 0.02) {
    discard;
  }

  const float speedT = clamp(inSpeed / 0.11, 0.0, 1.0);
  const float vortT = clamp(inVorticity * 0.42, 0.0, 1.0);
  const float t = clamp(max(speedT, vortT * 0.85), 0.0, 1.0);
  const vec3 color = Heatmap(t);
  if (frame.debugView == DebugViewVelocity) {
    outColor = vec4(color, 1.0);
    return;
  }
  outColor = vec4(color, mix(0.42, 0.82, t) * alpha);
}
