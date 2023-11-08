#version 450
#define PI 3.141592653589

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;

layout(binding = 0) uniform sampler2D depthImage;
layout(binding = 1, rgba32f) uniform image2D blurImage;

layout(binding = 2) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float farPlaneDistance;
  float particleRadius;
  vec3 pad;
}
model;

layout(binding = 3) uniform BlurSettings {
  vec2 blurDir;  
  float sigma;
  float kernelSize;
  float w_sum;
  vec3 pad;
}
blurSettings;

layout(binding = 4, rgba32f) uniform image2D bluredVolumeImage;
layout(binding = 5) uniform sampler2D volumeImage;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

int depthOrVolumeImage = 0;

float imLoad(vec2 texCoord) {
  ivec2 texel =
      ivec2(texCoord.x * model.windowSize.x, texCoord.y * model.windowSize.y);
  if (depthOrVolumeImage == 0){
    return imageLoad(blurImage, texel).x;
  } else {
    return imageLoad(bluredVolumeImage, texel).x;
  }
}

float loadDepth(vec2 texCoord) {
  if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0 )
    return 1.0f;

  if (constants.stageIndex == 0){
    if (depthOrVolumeImage == 0) {
      return texture(depthImage, texCoord).x;
    } else {
      return texture(volumeImage, texCoord).x;
    }
  } else {
    return imLoad(texCoord);
  }  
}

float gaussian(float x, float sigma) {
  return (1.0 / (sqrt(2.0 * PI) * sigma)) * exp(-(x * x) / (2.0 * sigma * sigma));
}

float getBluredDepth(vec2 dir) {
  float result = 0.0f;
  for (float i = -blurSettings.kernelSize; i <= blurSettings.kernelSize; i += 1.0) {
    float w = gaussian(i, blurSettings.sigma) / blurSettings.w_sum;
    if (isnan(w) || w > 1.0f || w < 0.0f)
      w = 0.0f;

    float d = loadDepth(coord + dir * i);
    result += d * w;
  }

  return result; 
}


void main() {
  ivec2 texel =
      ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);

  if (constants.stageIndex == 0) {
    float depthValue = getBluredDepth(vec2(blurSettings.blurDir.x / model.windowSize.x, 0.0f));
    imageStore(blurImage, texel, vec4(vec3(depthValue), 1.0));
    
    depthOrVolumeImage = 1;
    float volumeValue = getBluredDepth(vec2(blurSettings.blurDir.x / model.windowSize.x, 0.0f));
     imageStore(bluredVolumeImage, texel, vec4(vec3(volumeValue), 1.0));
  } else {
    float depthValue = getBluredDepth(vec2(0.0f, blurSettings.blurDir.y / model.windowSize.y));
    imageStore(blurImage, texel, vec4(vec3(depthValue), 1.0));

    depthOrVolumeImage = 1;
    float volumeValue = getBluredDepth(vec2(0.0f, blurSettings.blurDir.y / model.windowSize.y));
    imageStore(bluredVolumeImage, texel, vec4(vec3(volumeValue), 1.0));
  }

  outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}