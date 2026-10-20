#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  vec3 center;
  float radius;
};

layout(location = 0) out vec3 outViewPos;
layout(location = 1) out vec3 outViewCenter;
layout(location = 2) flat out float outRadius;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  const vec3 viewCenter = (frame.view * vec4(center, 1.0)).xyz;
  const vec2 corner = kCorners[gl_VertexIndex];
  const vec3 viewPos = viewCenter + vec3(corner * (radius * 2.0), 0.0);
  outViewPos = viewPos;
  outViewCenter = viewCenter;
  outRadius = radius;
  gl_Position = frame.projection * vec4(viewPos, 1.0);
}
