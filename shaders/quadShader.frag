#version 450
#define PI 3.1415926538

float maxDepth = 0.99f;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;

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

float texelSizeX;
float texelSizeY;

vec3 getNormal() {
  float offset = 0.005;

  float leftDepth = texture(blurImage, coord - vec2(texelSizeX, 0.0)).x;
  float rightDepth = texture(blurImage, coord + vec2(texelSizeX, 0.0)).x;
  float topDepth = texture(blurImage, coord - vec2(0.0, texelSizeY)).x;
  float bottomDepth = texture(blurImage, coord + vec2(0.0, texelSizeY)).x;

  vec3 leftViewPos = vec3(coord - vec2(texelSizeX, 0.0), leftDepth);
  vec3 rightViewPos = vec3(coord + vec2(texelSizeX, 0.0), rightDepth);
  vec3 topViewPos = vec3(coord - vec2(0.0, texelSizeY), topDepth);
  vec3 bottomViewPos = vec3(coord + vec2(0.0, texelSizeY), bottomDepth);

  vec3 dx = rightViewPos - leftViewPos;
  vec3 dy = bottomViewPos - topViewPos;

  vec3 viewSpaceNormal = normalize(cross(dx, dy));

  return viewSpaceNormal;
}

void main() {
  texelSizeX = 1.0f / model.windowSize.x;
  texelSizeY = 1.0f / model.windowSize.y;
  float originalDepth = texture(depthImage, coord).x;

  float blurValue = texture(blurImage, coord).x;
  if (blurValue > 0.98) {
	discard;
  } else {
	//outColor = vec4(getNormal(), 1.0);
	outColor = vec4(vec3(blurValue), 1.0);
  }
}