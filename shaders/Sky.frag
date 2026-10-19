#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Screen.glsl"

layout(set = 0, binding = 0) uniform sampler2D hdri;

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 viewDir = normalize(ViewDir(inUv, frame.projection));
  const vec3 worldDir = normalize(mat3(frame.inverseView) * viewDir);
  outColor = vec4(SampleEquirect(hdri, worldDir), 1.0);
}
