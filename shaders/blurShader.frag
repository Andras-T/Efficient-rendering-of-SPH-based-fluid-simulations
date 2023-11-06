#version 450
#define PI 3.141592653589

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;

layout(binding = 0) uniform sampler2D depthImage;
layout(binding = 1, rgba8) uniform image2D blurImage;

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
  float blurScale;
  float blurDepthFalloff;
  float filterRadius;
  float sigma;
  float kernelSize;
  float pad;
}
blurSettings;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

float getBluredDepthX(float filterRadius, vec2 blurDir, float blurScale,
                      float blurDepthFalloff) {
  float depth = texture(depthImage, coord).x;

  float sum = 0;
  float wsum = 0;
  for (float x = -filterRadius; x <= filterRadius; x += 1.0f) {
    float depthSample = texture(depthImage, coord + blurDir * x).x;

    float r = x * blurScale;
    float w = exp(-r * r);

    float r2 = (depthSample - depth) * blurDepthFalloff;
    float g = exp(-r2 * r2);

    sum += depthSample * w * g;
    wsum += w * g;
  }
  if (wsum > 0.0f) {
    sum /= wsum;
  }

  return sum;
}

float getBluredDepthY(ivec2 texel, float filterRadius, vec2 blurDir,
                      float blurScale, float blurDepthFalloff) {
  float depth = imageLoad(blurImage, texel).x;

  float sum = 0;
  float wsum = 0;
  for (float x = -filterRadius; x <= filterRadius; x += 1.0f) {
    vec2 blurCoord = coord + blurDir * x;
    ivec2 texCoord = ivec2(blurCoord.x * model.windowSize.x,
                           blurCoord.y * model.windowSize.y);
    float depthSample = imageLoad(blurImage, texCoord).x;

    float r = x * blurScale;
    float w = exp(-r * r);

    float r2 = (depthSample - depth) * blurDepthFalloff;
    float g = exp(-r2 * r2);

    sum += depthSample * w * g;
    wsum += w * g;
  }
  if (wsum > 0.0f) {
    sum /= wsum;
  }

  return sum;
}

float imLoad(vec2 texCoord) {
  ivec2 texel =
      ivec2(texCoord.x * model.windowSize.x, texCoord.y * model.windowSize.y);
  return imageLoad(blurImage, texel).x;
}

float loadDepth(vec2 texCoord) {
  if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0 )
    return 1.0f;

  float depth = 0.0f;

  if (constants.stageIndex == 0){
    depth = texture(depthImage, texCoord).x;
  } else {
    depth = imLoad(texCoord);
  }  

  return depth;
}

float gaussian(float x, float sigma) {
  return (1.0 / (sqrt(2.0 * PI) * sigma)) * exp(-(x * x) / (2.0 * sigma * sigma));
}

float getBluredDepth(vec2 dir) {
  float w_sum = 0.0f;
  for (float i = -blurSettings.kernelSize; i <= blurSettings.kernelSize; i += 1.0) {
      w_sum += gaussian(i, blurSettings.sigma);
  }

  float result = 0.0f;
  for (float i = -blurSettings.kernelSize; i <= blurSettings.kernelSize; i += 1.0) {
    float w = gaussian(i, blurSettings.sigma) / w_sum;
    if (isnan(w) || w > 1.0f || w < 0.0f)
      w = 0.0f;

    float d = loadDepth(coord + dir * i);
      result += d * w;
  }


  return result; 
}


void main() {
  float texelSizeX = 1.0 / model.windowSize.x;
  float texelSizeY = 1.0 / model.windowSize.y;

  ivec2 texel =
      ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);

  if (constants.stageIndex == 0) {
    float depthValue = getBluredDepth(vec2(blurSettings.blurDir.x / model.windowSize.x, 0.0f));
    imageStore(blurImage, texel, vec4(vec3(depthValue), 1.0));
  } else {
    float depthY =
        getBluredDepthY(texel, blurSettings.filterRadius,
                        vec2(0.0f, blurSettings.blurDir.x / model.windowSize.y),
                        blurSettings.blurScale, blurSettings.blurDepthFalloff);
    float depthValue = getBluredDepth(vec2(0.0f, blurSettings.blurDir.y / model.windowSize.y));
    imageStore(blurImage, texel, vec4(vec3(depthValue), 1.0));
  }

  outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}