#version 450

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

float getBluredDepthX(vec2 texcoord, float filterRadius, vec2 blurDir,
                     float blurScale, float blurDepthFalloff) {
  float depth = texture(depthImage, texcoord).x;

  float sum = 0;
  float wsum = 0;
  for (float x = -filterRadius; x <= filterRadius; x += 1.0f) {
    float depthSample = texture(depthImage, texcoord + blurDir * x).x;

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
    ivec2 texCoord =
      ivec2(blurCoord.x * model.windowSize.x, blurCoord.y * model.windowSize.y);
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

void main() {
  ivec2 imageSize = imageSize(blurImage);
  ivec2 texel =
      ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);
  vec4 pixel = imageLoad(blurImage, texel);
  
  if (constants.stageIndex == 0) {
    float depthX = getBluredDepthX(
      coord, 15.0f, vec2(1.0 / model.windowSize.x, 0.0f), 0.001f, 0.0f);
    imageStore(blurImage, texel, vec4(vec3(depthX), 1.0));
  } else {
    float depthY = getBluredDepthY(
      texel, 15.0f, vec2(0.0f, 1.0 / model.windowSize.y), 0.001f, 0.0f);
      imageStore(blurImage, texel, vec4(vec3(depthY), 1.0));
  }


  outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}