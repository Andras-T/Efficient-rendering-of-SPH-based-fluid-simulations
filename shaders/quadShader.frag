#version 450
#define PI 3.1415926538

float maxDepth = 0.99f;

struct Light {
  vec3 position;
  vec3 color;
};

Light headLight;

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
  float farPlaneDistance;
}
model;

layout(binding = 3, rgba32f) uniform image2D blurImage;

float texelSizeX;
float texelSizeY;

layout(binding = 4) uniform ViewMode {
  int mode;
}
viewMode;

float getImage2DValue(vec2 imageCoord) {
  ivec2 iCoord = ivec2(imageCoord.x * model.windowSize.x,
                                    imageCoord.y * model.windowSize.y);
  return imageLoad(blurImage, iCoord).x;
}

vec3 getOriginalNormal() {
  float leftDepth = texture(depthImage, coord - vec2(texelSizeX, 0.0)).x *
                    model.farPlaneDistance;
  float rightDepth = texture(depthImage, coord + vec2(texelSizeX, 0.0)).x *
                     model.farPlaneDistance;
  float topDepth = texture(depthImage, coord - vec2(0.0, texelSizeY)).x *
                   model.farPlaneDistance;
  float bottomDepth = texture(depthImage, coord + vec2(0.0, texelSizeY)).x *
                      model.farPlaneDistance;

  vec3 leftViewPos = vec3(coord - vec2(texelSizeX, 0.0), leftDepth);
  vec3 rightViewPos = vec3(coord + vec2(texelSizeX, 0.0), rightDepth);
  vec3 topViewPos = vec3(coord - vec2(0.0, texelSizeY), topDepth);
  vec3 bottomViewPos = vec3(coord + vec2(0.0, texelSizeY), bottomDepth);

  vec3 dx = rightViewPos - leftViewPos;
  vec3 dy = bottomViewPos - topViewPos;

  vec3 viewSpaceNormal = (vec4(normalize(cross(dx, dy)), 0.0)).xyz;
  return viewSpaceNormal;
}

vec3 getNormal() {
  float leftDepth =
      getImage2DValue(coord - vec2(texelSizeX, 0.0)) * model.farPlaneDistance;
  float rightDepth =
      getImage2DValue(coord + vec2(texelSizeX, 0.0)) * model.farPlaneDistance;
  float topDepth =
      getImage2DValue(coord - vec2(0.0, texelSizeY)) * model.farPlaneDistance;
  float bottomDepth =
      getImage2DValue(coord + vec2(0.0, texelSizeY)) * model.farPlaneDistance;

  vec3 leftViewPos = vec3(coord - vec2(texelSizeX, 0.0), leftDepth);
  vec3 rightViewPos = vec3(coord + vec2(texelSizeX, 0.0), rightDepth);
  vec3 topViewPos = vec3(coord - vec2(0.0, texelSizeY), topDepth);
  vec3 bottomViewPos = vec3(coord + vec2(0.0, texelSizeY), bottomDepth);

  vec3 dx = rightViewPos - leftViewPos;
  vec3 dy = bottomViewPos - topViewPos;

  vec3 viewSpaceNormal = (vec4(normalize(cross(dx, dy)), 0.0)).xyz;
  return viewSpaceNormal;
}

void main() {
  texelSizeX = 1.0f / model.windowSize.x;
  texelSizeY = 1.0f / model.windowSize.y;
  float originalDepth = texture(depthImage, coord).x;

  // headLight.color = vec3()

  float blurValue = getImage2DValue(coord);
  if (viewMode.mode == 0) {         // Original Depth
    if (originalDepth > 0.97) {
      discard;
    }
    outColor = vec4(vec3(originalDepth), 1.0);
  } else if (viewMode.mode == 1) {  // Original Normal
    if (originalDepth > 0.97) {
      discard;
    }
    outColor = vec4(getOriginalNormal() * 0.5 + vec3(0.5), 1.0);
  } else if (viewMode.mode == 2) {  // Blured Depth
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(vec3(blurValue), 1.0);
  } else if (viewMode.mode == 3) {  // Blured Normal
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(getNormal() * 0.5 + vec3(0.5), 1.0);
  } else if (viewMode.mode == 4) {  // Blured With Color
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(clamp(blurValue, 0.2, 0.75) * model.color.xyz, 1.0);
  }
}