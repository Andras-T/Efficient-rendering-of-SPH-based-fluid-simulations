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
  const vec3 halfway = normalize(light + viewDir);
  const float specular = pow(max(dot(normal, halfway), 0.0), 48.0);
  const vec3 color = inColor * (0.32 + 0.78 * diffuse) + vec3(0.28 * specular);
  outColor = vec4(color, 1.0);
}
