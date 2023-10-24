#version 450
#define PI 3.1415926538

float maxDepth = 0.99f;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D depthImage;
layout(binding = 1) uniform MVP {
  mat4 model;
  mat4 view;
  mat4 proj;
  vec3 cameraPos;
  float deltaTime;
}
mvp;

layout(binding = 2) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float pad2;
}
model;

layout(binding = 3) uniform sampler2D blurImage;

void main() {
  float texelSizeX = 1.0f / model.windowSize.x;
  float texelSizeY = 1.0f / model.windowSize.y;
  float originalDepth = texture(depthImage, fragTexCoord).x;

  float blurValue = texture(blurImage, fragTexCoord).x;
  if (blurValue > 0.98)
	discard;
  else
	outColor = vec4(0.0, 0.0, 1.0 * blurValue, 1.0);
}