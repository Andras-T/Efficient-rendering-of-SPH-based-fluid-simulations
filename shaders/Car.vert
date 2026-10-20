#version 460
#extension GL_GOOGLE_include_directive : require

#include "Types.glsl"
#include "Wind.glsl"

layout(push_constant, scalar) uniform PushConstants {
  FrameBuffer frame;
  MeshBuffer mesh;
};

layout(location = 0) out vec3 outWorld;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;

void main() {
  const MeshVertex vertex = mesh.vertices[gl_VertexIndex];
  outWorld = vertex.position;
  outNormal = vertex.normal;
  outColor = vertex.color;
  gl_Position = frame.projection * frame.view * vec4(vertex.position, 1.0);
}
