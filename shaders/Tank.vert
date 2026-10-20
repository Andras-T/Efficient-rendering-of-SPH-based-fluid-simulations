#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) out vec3 outWorld;
layout(location = 1) out vec3 outNormal;

const vec2 kQuad[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                             vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
  const uint face = gl_VertexIndex / 6u;
  const vec2 uv = kQuad[gl_VertexIndex % 6u];
  const vec3 mn = frame.domainMin;
  const vec3 mx = frame.domainMax;
  vec3 position = mn;
  vec3 normal = vec3(0.0, 1.0, 0.0);

  if (face == 0u) {
    position = vec3(mn.x, mix(mn.y, mx.y, uv.y), mix(mn.z, mx.z, uv.x));
    normal = vec3(-1.0, 0.0, 0.0);
  } else if (face == 1u) {
    position = vec3(mx.x, mix(mn.y, mx.y, uv.y), mix(mx.z, mn.z, uv.x));
    normal = vec3(1.0, 0.0, 0.0);
  } else if (face == 2u) {
    position = vec3(mix(mx.x, mn.x, uv.x), mix(mn.y, mx.y, uv.y), mn.z);
    normal = vec3(0.0, 0.0, -1.0);
  } else if (face == 3u) {
    position = vec3(mix(mn.x, mx.x, uv.x), mix(mn.y, mx.y, uv.y), mx.z);
    normal = vec3(0.0, 0.0, 1.0);
  } else {
    position = vec3(mix(mn.x, mx.x, uv.x), mn.y, mix(mn.z, mx.z, uv.y));
    normal = vec3(0.0, -1.0, 0.0);
  }

  outWorld = position;
  outNormal = normal;
  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
