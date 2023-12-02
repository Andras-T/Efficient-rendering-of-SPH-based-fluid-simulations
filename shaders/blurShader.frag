#version 450
#define PI 3.141592653589

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;

layout(binding = 0) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float farPlaneDistance;
  float particleRadius;
  vec3 pad;
}
model;

layout(binding = 1) uniform BlurSettings {
  vec2 blurDir;  
  float sigma;
  float kernelSize;
  float w_sum;
  vec3 pad;
}
blurSettings;
layout(binding = 2) uniform sampler2D depthImage;
layout(binding = 3, r32f) uniform image2D blurImage1;
layout(binding = 4, r32f) uniform image2D blurImage2;
layout(binding = 5) uniform usampler2D volumeImage; 
layout(binding = 6, r32f) uniform image2D bluredVolumeImage1;
layout(binding = 7, r32f) uniform image2D bluredVolumeImage2;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

int depthOrVolumeImage = 0;

float imLoad(vec2 texCoord) {
  ivec2 texel =
      ivec2(texCoord.x * model.windowSize.x, texCoord.y * model.windowSize.y);
  if (depthOrVolumeImage == 0){
      return imageLoad(blurImage1, texel).x;
  } else {
      return imageLoad(bluredVolumeImage1, texel).x;
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
  float center = loadDepth(coord);
  float result = 0.0f;
  for (float i = -blurSettings.kernelSize; i <= blurSettings.kernelSize; i += 1.0) {
    float w = gaussian(i, blurSettings.sigma) / blurSettings.w_sum;
    if (isnan(w) || w > 1.0f || w < 0.0f)
      w = 0.0f;

    float d = loadDepth(coord + dir * i);
    //if (center > 0.995f)
    //  w *= 0.9f;

    //float diff = center - d;
  
    result += d * w;
  }

  return result; 
}

float loadVolume(vec2 texCoord) {
  if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0 )
    return 0;

  if (constants.stageIndex == 0){
    return float(texture(volumeImage, texCoord).x);
  } else {
    ivec2 texel =
      ivec2(texCoord.x * model.windowSize.x, texCoord.y * model.windowSize.y);
    return float(imageLoad(bluredVolumeImage1, texel).x);
  }  
}

float getBluredVolume(vec2 dir) {
  float center = loadVolume(coord);
  float result = 0.0f;
  for (float i = -blurSettings.kernelSize; i <= blurSettings.kernelSize; i += 1.0) {
    float w = gaussian(i, blurSettings.sigma) / blurSettings.w_sum;
    if (isnan(w) || w > 1.0f || w < 0.0f)
      w = 0.0f;

    float d = loadVolume(coord + dir * i);
    //if (center > 0.995f)
    //  w *= 0.9f;

    result += float(d) * w;
  }

  return result; 
}


void main() {
  ivec2 texel =
      ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);

  if (constants.stageIndex == 0) {
    float depthValue = getBluredDepth(vec2(blurSettings.blurDir.x / model.windowSize.x, 0.0f));
    imageStore(blurImage1, texel, vec4(vec3(depthValue), 1.0));
    
    depthOrVolumeImage = 1;
    float volumeValue = getBluredVolume(vec2(blurSettings.blurDir.x / model.windowSize.x, 0.0f));
    imageStore(bluredVolumeImage1, texel, vec4(volumeValue));
  } else {
    float depthValue = getBluredDepth(vec2(0.0f, blurSettings.blurDir.y / model.windowSize.y));
    imageStore(blurImage2, texel, vec4(vec3(depthValue), 1.0));
    
    depthOrVolumeImage = 1;
    float volumeValue = getBluredVolume(vec2(0.0f, blurSettings.blurDir.y / model.windowSize.y));
    imageStore(bluredVolumeImage2, texel, vec4(clamp(volumeValue, 1.0f, 20.0f) / 20.0f));
  }

  outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}