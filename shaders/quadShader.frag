#version 450
#define PI 3.1415926538

layout(location = 0) out vec4 outColor;

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

layout(location = 1) in vec2 fragTexCoord;

float getBluredDepth(vec2 texcoord, float filterRadius, vec2 blurDir,
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

//vec3 getNormal(vec2 texcoord) {
//  float offset = 0.005;
//
//  float leftDepth = texture(depthImage, texcoord - vec2(offset, 0.0)).x;
//  float rightDepth = texture(depthImage, texcoord + vec2(offset, 0.0)).x;
//  float topDepth = texture(depthImage, texcoord - vec2(0.0, offset)).x;
//  float bottomDepth = texture(depthImage, texcoord + vec2(0.0, offset)).x;
//
//  vec3 leftViewPos = vec3(texcoord - vec2(offset, 0.0), leftDepth);
//  vec3 rightViewPos = vec3(texcoord + vec2(offset, 0.0), rightDepth);
//  vec3 topViewPos = vec3(texcoord - vec2(0.0, offset), topDepth);
//  vec3 bottomViewPos = vec3(texcoord + vec2(0.0, offset), bottomDepth);
//
//  vec3 dx = rightViewPos - leftViewPos;
//  vec3 dy = bottomViewPos - topViewPos;
//
//  vec3 viewSpaceNormal = normalize(cross(dx, dy));
//
//  return viewSpaceNormal;
//}

float maxDepth = 1.0f;
float texelSize;

vec3 uvToEye(vec2 uv, float depth) {
    // Convert depth to eye-space position
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 eyePos = inverse(mvp.proj) * clipPos;
    return eyePos.xyz / eyePos.w;
}

vec3 getEyePos(sampler2D depthTex, vec2 texCoord) {
    // Read depth from the depth texture
    float depth = texture(depthTex, texCoord).r;
    // Calculate eye-space position from depth
    return uvToEye(texCoord, depth);
}

vec3 getNormal(vec2 texCoord) {
    // Read eye-space depth from texture
    float depth = texture(depthImage, texCoord).r;
    if (depth > maxDepth) {
        discard;
    }

    // Calculate eye-space position from depth
    vec3 posEye = uvToEye(texCoord, depth);

    // Calculate differences
    vec3 ddx = getEyePos(depthImage, texCoord + vec2(texelSize, 0)) - posEye;
    vec3 ddx2 = posEye - getEyePos(depthImage, texCoord + vec2(-texelSize, 0));
    if (abs(ddx.z) > abs(ddx2.z)) {
        ddx = ddx2;
    }

    vec3 ddy = getEyePos(depthImage, texCoord + vec2(0, texelSize)) - posEye;
    vec3 ddy2 = posEye - getEyePos(depthImage, texCoord + vec2(0, -texelSize));
    if (abs(ddy.z) > abs(ddy2.z)) {
        ddy = ddy2;
    }

    // Calculate normal
    vec3 n = cross(ddx, ddy);
    return normalize(-n);

}

void main() {
  texelSize = 1.0f/model.windowSize.x;
  float originalDepth = texture(depthImage, fragTexCoord).x;

  float depthX =
      getBluredDepth(fragTexCoord, 25.0f, vec2(0.01f, 0.0f), 0.0001f, 0.00001);

  //outColor = vec4(vec3(originalDepth), 1.0f);
  outColor = vec4((getNormal(fragTexCoord) - vec3(0.5f)) * 2.0f, 1.0f);
  if (originalDepth == 1.0f) {
    outColor = vec4(0.1, 0.1, 0.1, 1.0f);
  }
}