#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) out vec2 coord;
layout(location = 2) out vec3 cubePos;

layout(binding = 1) uniform MVP {
  mat4 model;
  mat4 view;
  mat4 proj;
  mat4 viewModel;
  mat4 inverseModel;
  mat4 inverseProj;
  vec3 cameraPos;
  float deltaTime;
}
mvp;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

mat4 removeTranslation(mat4 mat) {
  mat[3][0] = 0.0;
  mat[3][1] = 0.0;
  mat[3][2] = 0.0;
  return mat;
}

mat4 scaleMatrix(vec3 scale) {
  return mat4(vec4(scale.x, 0.0, 0.0, 0.0), vec4(0.0, scale.y, 0.0, 0.0),
              vec4(0.0, 0.0, scale.z, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
}

// Todo: give correct coords to the TexCube
void main() {
  if (constants.stageIndex == 0) {
    cubePos = inPosition.xyz;
    vec4 pos =
        mvp.proj * removeTranslation(mvp.view) * vec4(inPosition.xyz, 1.0);
    gl_Position = pos;
  } else {
    gl_Position = scaleMatrix(vec3(1.0, 1.0, 1.0)) * inPosition;
    coord = (inPosition.xy + vec2(1.0f)) * 0.5f;
  }
}