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

layout(binding = 3) uniform BlurSettings {
  float blurScale;
  float blurDepthFalloff;
  float filterRadius;
  float pad1;
  vec2 blurDir;
  vec2 pad2;
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

void main() {
  ivec2 imageSize = imageSize(blurImage);
  ivec2 texel =
      ivec2(coord.x * model.windowSize.x, coord.y * model.windowSize.y);
  vec4 pixel = imageLoad(blurImage, texel);

  if (constants.stageIndex == 0) {
    float depthX = getBluredDepthX(
        blurSettings.filterRadius,
        2.0 * vec2(blurSettings.blurDir.y / model.windowSize.x, 0.0f),
        blurSettings.blurScale, blurSettings.blurDepthFalloff);
    imageStore(blurImage, texel, vec4(vec3(depthX), 1.0));
  } else {
    float depthY =
        getBluredDepthY(texel, blurSettings.filterRadius,
                        vec2(0.0f, blurSettings.blurDir.x / model.windowSize.y),
                        blurSettings.blurScale, blurSettings.blurDepthFalloff);
    imageStore(blurImage, texel, vec4(vec3(depthY), 1.0));
  }

  outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}