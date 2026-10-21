#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Wind.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  MeshBuffer mesh;
};

layout(location = 0) in vec3 inWorld;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 normal = normalize(inNormal);
  const vec3 light = normalize(frame.lightDirection);
  const float diffuse = max(dot(normal, light), 0.0);
  const vec3 viewDir = normalize(frame.inverseView[3].xyz - inWorld);
  const float rim = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
  const float shade = 0.40 + 0.55 * diffuse;
  outColor = vec4(inColor * shade + inColor * rim * 0.10, 1.0);
}
