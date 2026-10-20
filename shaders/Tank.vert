#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) out vec3 outWorld;
layout(location = 1) out vec3 outNormal;

// Six faces, two CCW triangles each, outward normals. Closed glass cube.
const vec3 kUnit[36] = vec3[](
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0),
    vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 0.0));

const vec3 kNormal[6] = vec3[](vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0),
                               vec3(0.0, 1.0, 0.0), vec3(0.0, -1.0, 0.0),
                               vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, -1.0));

const uint CylinderSegments = 48u;
const float TwoPi = 6.28318530718;
const float Pi = 3.14159265359;

vec2 PrismRing(uint i, uint n, float ang0, vec2 center, float radius) {
  const float ang = ang0 + TwoPi * float(i) / float(n);
  return center + radius * vec2(cos(ang), sin(ang));
}

void PrismVertex(uint index, uint n, float ang0, out vec3 position, out vec3 normal) {
  const vec3 mn = frame.domainMin;
  const vec3 mx = frame.domainMax;
  const vec2 center = 0.5 * (mn.xz + mx.xz);
  const float radius = 0.5 * min(mx.x - mn.x, mx.z - mn.z);
  const uint i0 = index / 6u;
  if (index < 6u * n) {
    const uint seg = i0;
    const uint corner = index % 6u;
    const uint ia = (corner == 1u || corner == 2u || corner == 4u) ? ((seg + 1u) % n) : seg;
    const float y01 = (corner == 2u || corner == 4u || corner == 5u) ? 1.0 : 0.0;
    const vec2 xz = PrismRing(ia, n, ang0, center, radius);
    position = vec3(xz.x, mix(mn.y, mx.y, y01), xz.y);
    const vec2 a = PrismRing(seg, n, ang0, center, radius);
    const vec2 b = PrismRing((seg + 1u) % n, n, ang0, center, radius);
    const vec2 edge = b - a;
    const vec2 outward = normalize(vec2(edge.y, -edge.x));
    normal = vec3(outward.x, 0.0, outward.y);
    return;
  }
  if (index < 9u * n) {
    const uint tri = (index - 6u * n) / 3u;
    const uint corner = (index - 6u * n) % 3u;
    vec2 xz = center;
    if (corner == 1u) {
      xz = PrismRing((tri + 1u) % n, n, ang0, center, radius);
    } else if (corner == 2u) {
      xz = PrismRing(tri, n, ang0, center, radius);
    }
    position = vec3(xz.x, mn.y, xz.y);
    normal = vec3(0.0, -1.0, 0.0);
    return;
  }
  const uint tri = (index - 9u * n) / 3u;
  const uint corner = (index - 9u * n) % 3u;
  vec2 xz = center;
  if (corner == 1u) {
    xz = PrismRing(tri, n, ang0, center, radius);
  } else if (corner == 2u) {
    xz = PrismRing((tri + 1u) % n, n, ang0, center, radius);
  }
  position = vec3(xz.x, mx.y, xz.y);
  normal = vec3(0.0, 1.0, 0.0);
}

void main() {
  vec3 position;
  vec3 normal;
  if (frame.tankShape == TankCylinder || frame.tankShape == TankHexagon) {
    const uint n = frame.tankShape == TankHexagon ? 6u : CylinderSegments;
    const float ang0 = frame.tankShape == TankHexagon ? (Pi / float(n)) : 0.0;
    PrismVertex(uint(gl_VertexIndex), n, ang0, position, normal);
  } else {
    const vec3 unit = kUnit[gl_VertexIndex];
    position = mix(frame.domainMin, frame.domainMax, unit);
    normal = kNormal[gl_VertexIndex / 6];
  }
  outWorld = position;
  outNormal = normal;
  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
