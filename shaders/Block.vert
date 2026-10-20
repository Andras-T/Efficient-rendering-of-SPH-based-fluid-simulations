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

const vec2 kQuad[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                             vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
  const uint face = gl_VertexIndex / 6u;
  const vec2 uv = kQuad[gl_VertexIndex % 6u];
  vec3 position = bmin;
  vec3 normal = vec3(0.0, 1.0, 0.0);

  if (face == 0u) {
    position = vec3(bmin.x, mix(bmin.y, bmax.y, uv.y), mix(bmax.z, bmin.z, uv.x));
    normal = vec3(-1.0, 0.0, 0.0);
  } else if (face == 1u) {
    position = vec3(bmax.x, mix(bmin.y, bmax.y, uv.y), mix(bmin.z, bmax.z, uv.x));
    normal = vec3(1.0, 0.0, 0.0);
  } else if (face == 2u) {
    position = vec3(mix(bmax.x, bmin.x, uv.x), mix(bmin.y, bmax.y, uv.y), bmin.z);
    normal = vec3(0.0, 0.0, -1.0);
  } else if (face == 3u) {
    position = vec3(mix(bmin.x, bmax.x, uv.x), mix(bmin.y, bmax.y, uv.y), bmax.z);
    normal = vec3(0.0, 0.0, 1.0);
  } else if (face == 4u) {
    position = vec3(mix(bmin.x, bmax.x, uv.x), bmin.y, mix(bmin.z, bmax.z, uv.y));
    normal = vec3(0.0, -1.0, 0.0);
  } else {
    position = vec3(mix(bmin.x, bmax.x, uv.x), bmax.y, mix(bmax.z, bmin.z, uv.y));
    normal = vec3(0.0, 1.0, 0.0);
  }

  outWorld = position;
  outNormal = normal;
  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
