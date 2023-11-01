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
  int viewOrWorldSpace;
  float shininess;
  float ambient;
  float lightStrength;
  float lightFOV;
}
viewMode;

float getImage2DValue(vec2 imageCoord) {
  ivec2 iCoord = ivec2(imageCoord.x * model.windowSize.x,
                                    imageCoord.y * model.windowSize.y);
  return imageLoad(blurImage, iCoord).x;
}

vec3 getWorldPos(vec2 texCoord, float depth) {
  vec4 clip = vec4(texCoord * 2.0 - vec2(1.0, 1.0), depth, 1.0);
  vec4 view = inverse(mvp.proj) * clip;
  return (inverse(mvp.view) * vec4(view.xyz/ view.w, 1.0)).xyz;
}

vec3 getViewPos(vec2 texCoord, float depth) {
  vec4 clip = vec4(texCoord * 2.0 - vec2(1.0, 1.0), depth, 1.0);
  vec4 view = inverse(mvp.proj) * clip;
  return vec3(view.xyz/ view.w);
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

  vec3 leftWorldPos;
  vec3 rightWorldPos;
  vec3 topWorldPos;
  vec3 bottomWorldPos;

  if (viewMode.viewOrWorldSpace == 1) {
    leftWorldPos = getWorldPos(coord - vec2(texelSizeX, 0.0), leftDepth);
    rightWorldPos = getWorldPos(coord + vec2(texelSizeX, 0.0), rightDepth);
    topWorldPos = getWorldPos(coord - vec2(0.0, texelSizeY), topDepth);
    bottomWorldPos = getWorldPos(coord + vec2(0.0, texelSizeY), bottomDepth);
  } else {
    leftWorldPos = getViewPos(coord - vec2(texelSizeX, 0.0), leftDepth);
    rightWorldPos = getViewPos(coord + vec2(texelSizeX, 0.0), rightDepth);
    topWorldPos = getViewPos(coord - vec2(0.0, texelSizeY), topDepth);
    bottomWorldPos = getViewPos(coord + vec2(0.0, texelSizeY), bottomDepth);
  }
  vec3 dx = rightWorldPos - leftWorldPos;
  vec3 dy = bottomWorldPos - topWorldPos;

  vec3 crossProduct = cross(dx, dy);

  vec3 worldSpaceNormal = crossProduct / length(crossProduct);
  return worldSpaceNormal;
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

  vec3 leftWorldPos;
  vec3 rightWorldPos;
  vec3 topWorldPos;
  vec3 bottomWorldPos;

  if (viewMode.viewOrWorldSpace == 1) {
    leftWorldPos = getWorldPos(coord - vec2(texelSizeX, 0.0), leftDepth);
    rightWorldPos = getWorldPos(coord + vec2(texelSizeX, 0.0), rightDepth);
    topWorldPos = getWorldPos(coord - vec2(0.0, texelSizeY), topDepth);
    bottomWorldPos = getWorldPos(coord + vec2(0.0, texelSizeY), bottomDepth);
  } else {
    leftWorldPos = getViewPos(coord - vec2(texelSizeX, 0.0), leftDepth);
    rightWorldPos = getViewPos(coord + vec2(texelSizeX, 0.0), rightDepth);
    topWorldPos = getViewPos(coord - vec2(0.0, texelSizeY), topDepth);
    bottomWorldPos = getViewPos(coord + vec2(0.0, texelSizeY), bottomDepth);
  }

  vec3 dx = rightWorldPos - leftWorldPos;
  vec3 dy = bottomWorldPos - topWorldPos;

  vec3 crossProduct = cross(dx, dy);

  vec3 worldSpaceNormal = crossProduct / length(crossProduct);
  return worldSpaceNormal;
}

vec3 calcLight(float depth) {
  vec3 outputColor = vec3(0.0);

  headLight.color = vec3(0.9, 0.9, 0.6);
  headLight.position = mvp.cameraPos;

  vec3 hitPosition = getWorldPos(coord, depth);
  vec3 viewDirection = getWorldPos(vec2(0.5, 0.5), 1.0);
  
  if (viewMode.viewOrWorldSpace == 0) {
    vec4 lightPos = mvp.view * vec4(mvp.cameraPos, 1.0);
    headLight.position = lightPos.xyz / lightPos.w;
    hitPosition = getViewPos(coord, depth);
    viewDirection = getViewPos(vec2(0.5, 0.5), 1.0);
  }

  vec3 lightDirection = normalize(hitPosition - headLight.position);
  
  float angle = acos(dot(normalize(lightDirection), normalize(viewDirection)));
  float maxAngle = viewMode.lightFOV / 180.0 * PI;
  if (angle < maxAngle) {
    vec3 halfVector = normalize(lightDirection + viewDirection);
    vec3 bluredNormal = getNormal();
    float diffuse = max(dot(bluredNormal, lightDirection), 0.0f);
    float specular = pow(max(dot(bluredNormal, halfVector), 0.0f), viewMode.shininess);

    outputColor += (diffuse + specular) * headLight.color * (1.0 - angle / maxAngle);
  }
  outputColor *= viewMode.lightStrength;
  outputColor += viewMode.ambient * model.color.xyz;
  return outputColor;
}

void main() {
  texelSizeX = 1.0f / model.windowSize.x;
  texelSizeY = 1.0f / model.windowSize.y;
  float originalDepth = texture(depthImage, coord).x;

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
  } else if (viewMode.mode == 5) {
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(calcLight(blurValue), 1.0);
  }
}

//vec3 calcLight(float depth) {
//  vec3 outputColor = vec3(0.0);
//
//  headLight.color = vec3(0.9, 0.9, 0.6);
//  headLight.position = vec3(0.5, 0.5, 0.0);
//  vec3 viewPos = vec3(coord, depth);          // might want to * model.farPlaneDistance
//
//  vec3 lightDirection = normalize(headLight.position - viewPos);
//  vec3 viewDirection = lightDirection;
//
//  vec3 bluredNormal = getNormal();
//
//  float diffuse = max(dot(bluredNormal, lightDirection), 0.0f);
//
//  vec3 halfVector = normalize(lightDirection + viewDirection);
//  float specular = pow(max(dot(bluredNormal, halfVector), 0.0f), viewMode.shininess);
//
//  outputColor += (diffuse + specular) * headLight.color;
//  outputColor += viewMode.ambient * model.color.xyz;
//  outputColor *= viewMode.lightStrength;
//
//  return outputColor;
//}