#version 450
#define PI 3.141592653589

float maxDepth = 0.96f;

struct Light {
  vec3 position;
  vec3 color;
};

Light headLight;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 coord;
layout(location = 2) in vec3 cubePos;

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
  float particleRadius;
  vec3 pad;
}
model;

layout(binding = 3, rgba32f) uniform image2D blurImage;

float texelSizeX;
float texelSizeY;

layout(binding = 4) uniform ViewMode {
  int mode;
  float shininess;
  float ambient;
  float lightStrength;
  float lightFOV;
  float reflection;
  float maxDepth;
  float pad2;
}
viewMode;

layout(push_constant) uniform Constants { int stageIndex; }
constants;

layout(binding = 5) uniform samplerCube cubemap;

float getImage2DValue(vec2 imageCoord) {
  ivec2 iCoord = ivec2(imageCoord.x * model.windowSize.x,
                       imageCoord.y * model.windowSize.y);
  return imageLoad(blurImage, iCoord).x;
}

mat4 removeTranslation(mat4 mat) {
  mat[3][0] = 0.0;
  mat[3][1] = 0.0;
  mat[3][2] = 0.0;
  return mat;
}

vec3 getViewPos(vec2 uv, float depth) {
  vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
  vec4 eyePos = inverse(mvp.proj) * clipPos;
  return eyePos.xyz / eyePos.w;
}

vec3 getEyePos(vec2 texCoord) {
  ivec2 iCoord =
      ivec2(texCoord.x * model.windowSize.x, texCoord.y * model.windowSize.y);
  float depth = getImage2DValue(texCoord);
  return getViewPos(texCoord, depth);
}

vec3 getNormal() {
  float depth = getImage2DValue(coord);
  if (depth > viewMode.maxDepth) {
    discard;
  }

  // Calculate eye-space position from depth
  vec3 posEye = getViewPos(coord, depth);

  // Calculate differences on X
  vec3 ddx = getEyePos(coord + vec2(texelSizeX, 0)) - posEye;
  vec3 ddx2 = posEye - getEyePos(coord + vec2(-texelSizeX, 0));
  if (abs(ddx.z) > abs(ddx2.z)) {
    ddx = ddx2;
  }
  // Calculate differences on Y
  vec3 ddy = getEyePos(coord + vec2(0, texelSizeY)) - posEye;
  vec3 ddy2 = posEye - getEyePos(coord + vec2(0, -texelSizeY));
  if (abs(ddy.z) > abs(ddy2.z)) {
    ddy = ddy2;
  }

  vec4 viewSpaceNormal = vec4(normalize(-cross(ddx, ddy)), 1.0);

  vec4 worldNormal = inverse(removeTranslation(mvp.view)) * viewSpaceNormal;
  worldNormal = inverse(mvp.model) * worldNormal;

  return normalize(worldNormal).xyz;
}

vec3 calcLight(float depth) {
  vec3 outputColor = vec3(0.0);

  headLight.color = vec3(0.9, 0.9, 0.6);
  headLight.position = (mvp.view * vec4(mvp.cameraPos, 1.0)).xyz;

  vec3 hitPosition = getViewPos(coord, depth);
  vec3 viewDirection = getViewPos(vec2(0.5, 0.5), 1.0);

  vec3 lightDirection = normalize(hitPosition - headLight.position);
  vec3 reflection = vec3(0.0f);

  float angle = acos(dot(normalize(lightDirection), normalize(viewDirection)));
  float maxAngle = viewMode.lightFOV / 180.0 * PI;
  vec3 normal = getNormal();
  if (angle < maxAngle) {
    vec3 halfVector = normalize(lightDirection + viewDirection);
    vec3 viewNormal = normalize((mvp.view * vec4(normal, 1.0)).xyz);

    float diffuse = max(dot(viewNormal, lightDirection), 0.0f);
    float specular =
        pow(max(dot(viewNormal, halfVector), 0.0f), viewMode.shininess);

    outputColor +=
        (diffuse + specular) * headLight.color * (1.0 - angle / maxAngle);
  }

  outputColor *= viewMode.lightStrength;
  outputColor += viewMode.ambient * model.color.xyz;

  vec3 R = reflect(lightDirection, normalize(normal));
  reflection = texture(cubemap, R).xyz;

  float reflectionScale = 0.0f;
  float pixel = 15.0f;
  if (getImage2DValue(coord + pixel * vec2(texelSizeX)) < viewMode.maxDepth)
    reflectionScale += 0.25f;
  if (getImage2DValue(coord - pixel * vec2(texelSizeX)) < viewMode.maxDepth)
    reflectionScale += 0.25f;
  if (getImage2DValue(coord + pixel * vec2(texelSizeY)) < viewMode.maxDepth)
    reflectionScale += 0.25f;
  if (getImage2DValue(coord - pixel * vec2(texelSizeY)) < viewMode.maxDepth)
    reflectionScale += 0.25f;

  if (normalize(normal).z < -0.25)
    return outputColor + reflectionScale * 0.3f * reflection * viewMode.reflection;
  
  return outputColor + reflectionScale * reflection * viewMode.reflection;
}

void main() {
  texelSizeX = 1.0 / model.windowSize.x;
  texelSizeY = 1.0 / model.windowSize.y;
  float originalDepth = texture(depthImage, coord).x;

  float blurValue = getImage2DValue(coord);
  if (viewMode.mode == 0 && constants.stageIndex == 1) {      // Original Depth
    if (originalDepth > 0.97) {
      discard;
    }
    outColor = vec4(vec3(originalDepth), 1.0);
  } else if (viewMode.mode == 1 && constants.stageIndex == 1) { // Blured Depth
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(vec3(blurValue), 1.0);
  } else if (viewMode.mode == 2 && constants.stageIndex == 1) { // Blured Normal
    if (originalDepth > 0.97) {
      discard;
    }
    outColor = vec4((getNormal() + vec3(0.5)) * 0.5, 1.0);
  } else if (viewMode.mode == 3 &&
             constants.stageIndex == 1) {                       // Blured With Color
    if (blurValue > 0.97) {
      discard;
    }
    outColor = vec4(clamp(blurValue, 0.2, 0.75) * model.color.xyz, 1.0);
  } else if (viewMode.mode == 4) {                              // Light & Background
    if (constants.stageIndex == 0) {
      outColor = vec4(vec3(texture(cubemap, cubePos).xyz), 1.0);
    } else {
      if (blurValue > 0.97) {
        discard;
      }
      vec3 light = calcLight(blurValue);
      outColor = vec4(light, 1.0);
    }
  } else {
    discard;
  }
}