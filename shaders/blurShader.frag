#version 450

float texelSizeX;
float texelSizeY;
ivec2 texel;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;

layout(binding = 0) uniform sampler2D depthImage;
layout(binding = 1, rgba8) uniform image2D blurImage;

layout(binding = 2) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float pad2;
}
model;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

float readDepthImage(vec2 texCoord, float currentSum, float weight) {
  if (texCoord.x < 0.0 || texCoord.y < 0.0 || texCoord.x > 1.0 || texCoord.y > 1.0)
    return currentSum;

  return currentSum + weight * texture(depthImage, texCoord).x; 
}

float getBluredDepth() {
  float originalDepth = texture(depthImage, coord).x; 

  float sum = 0.0f;  
  readDepthImage(coord, sum, 10.0/18.0);
  readDepthImage(coord + vec2(texelSizeX, 0.0), sum, 4.0/18.0);
  readDepthImage(coord - vec2(texelSizeX, 0.0), sum, 4.0/18.0);
  //readDepthImage(coord + vec2(0.0, texelSizeY), sum, 2.0/18.0);
  //readDepthImage(coord - vec2(0.0, texelSizeY), sum, 2.0/18.0);
  //readDepthImage(coord + vec2(texelSizeX, texelSizeY), sum, 1.0/18.0);
  //readDepthImage(coord - vec2(texelSizeX, texelSizeY), sum, 1.0/18.0);
  //readDepthImage(coord + vec2(-texelSizeX, texelSizeY), sum, 1.0/18.0);
  //readDepthImage(coord - vec2(-texelSizeX, texelSizeY), sum, 1.0/18.0);

    return sum;
}

void main() {
  //ivec2 imageSize = imageSize(blurImage);
  texelSizeX = 1.0f / model.windowSize.x;
  texelSizeY = 1.0f / model.windowSize.y;
  texel = ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);
  vec4 pixel = imageLoad(blurImage, texel);
  
  if (constants.stageIndex == 0) {
    imageStore(blurImage, texel, vec4(vec3(texture(depthImage, coord).x), 1.0));
  } else {}
  
  outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}