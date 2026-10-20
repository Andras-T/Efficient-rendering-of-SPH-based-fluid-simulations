#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  vec3 bmin;
  vec3 bmax;
};

layout(location = 0) out vec3 outWorld;
layout(location = 1) out vec3 outNormal;

const vec3 kUnit[6] =
    vec3[](vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0),
           vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0));

void main() {
  const vec3 unit = kUnit[gl_VertexIndex];
  const vec3 position = mix(bmin, bmax, unit);
  outWorld = position;
  outNormal = vec3(0.0, 1.0, 0.0);
  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
