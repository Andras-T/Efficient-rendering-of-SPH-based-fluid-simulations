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
  const float along = inCorner.x;
  const float across = inCorner.y;
  const float alpha = exp(-3.2 * along * along - 4.8 * across * across);
  if (alpha < 0.04) {
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
  outColor = vec4(color, mix(0.50, 0.88, t) * alpha);
}
