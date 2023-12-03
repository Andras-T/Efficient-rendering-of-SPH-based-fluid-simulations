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

layout(binding = 0) uniform MVP {
  mat4 model;
  mat4 view;
  mat4 proj;
  mat4 viewModel;
  mat4 inverseModel;
  mat4 inverseProj;
  vec3 cameraPos;
  float deltaTime;
  vec4 lightPosition;
}
mvp;

layout(binding = 1) uniform Model {
  vec4 color;
  vec4 wallColor;
  vec2 windowSize;
  int wall;
  float farPlaneDistance;
  float particleRadius;
  vec3 pad;
}
model;

float texelSizeX;
float texelSizeY;

layout(binding = 2) uniform ViewMode {
  int mode;
  float shininess;
  float ambient;
  float lightStrength;
  float lightFOV;
  float reflection;
  float maxDepth;
  float transparency;
  float refraction;
  float refractionStrength;
}
viewMode;

layout(binding = 3) uniform samplerCube cubemap;
layout(binding = 4) uniform sampler2D depthImage;
layout(binding = 5, r32f) uniform image2D blurImage;
layout(binding = 6, r32f) uniform image2D bluredVolumeImage;
layout(binding = 7) uniform usampler2D volumeImage;

layout(push_constant) uniform Constants { int stageIndex; }
constants;


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
  vec4 eyePos = mvp.inverseProj * clipPos;
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

  vec4 worldNormal = inverse((mvp.view)) * viewSpaceNormal;

  return normalize(worldNormal).xyz;
}

float SchlickFresnel(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calcLight() {
  vec3 outputColor = vec3(0.0);
  vec3 reflection = vec3(0.0f);

  headLight.color = vec3(0.9, 0.9, 0.6);
  headLight.position = mvp.lightPosition.xyz;

  vec3 forwardDirection = normalize((inverse((mvp.view)) * vec4(getViewPos(vec2(0.5, 0.5), 1.0), 1.0f)).xyz);

  vec4 viewPos = vec4(getEyePos(coord), 1.0f);
  vec4 worldPos = inverse((mvp.view)) * viewPos;
  vec3 viewDirection = normalize(worldPos.xyz - mvp.cameraPos);
  vec3 lightDirection = normalize(worldPos.xyz - headLight.position);

  //float angle = acos(dot(normalize(lightDirection), forwardDirection));
  //float maxAngle = viewMode.lightFOV / 180.0 * PI;
  vec3 normal = getNormal();
  //if (angle < maxAngle) {
    vec3 halfVector = normalize(lightDirection + viewDirection);

    float diffuse = max(dot(normal, lightDirection), 0.0f);
    float specular =
        pow(max(dot(normal, halfVector), 0.0f), viewMode.shininess);

    outputColor +=
        (diffuse + specular) * headLight.color;// * (1.0 - angle / maxAngle);
  //}

  outputColor *= viewMode.lightStrength;
  outputColor += viewMode.ambient * model.color.xyz;

  vec3 R = reflect(viewDirection, normal);
  reflection = texture(cubemap, R).xyz;

  float reflectionScale = 1.0f; // change it to 0
  //float pixel = 15.0f;
  //if (getImage2DValue(coord + pixel * vec2(texelSizeX)) < viewMode.maxDepth)
  //  reflectionScale += 0.25f;
  //if (getImage2DValue(coord - pixel * vec2(texelSizeX)) < viewMode.maxDepth)
  //  reflectionScale += 0.25f;
  //if (getImage2DValue(coord + pixel * vec2(texelSizeY)) < viewMode.maxDepth)
  //  reflectionScale += 0.25f;
  //if (getImage2DValue(coord - pixel * vec2(texelSizeY)) < viewMode.maxDepth)
  //  reflectionScale += 0.25f;

  float cosTheta = dot(normal, viewDirection);
  float F0 = 0.04;
  float fresnel = SchlickFresnel(cosTheta, F0);

  return outputColor + reflectionScale * reflection * viewMode.reflection + vec3(fresnel * 0.001);
}

void main() {
  texelSizeX = 1.0 / model.windowSize.x;
  texelSizeY = 1.0 / model.windowSize.y;
  float originalDepth = texture(depthImage, coord).x;

  float blurValue = getImage2DValue(coord);
  if (viewMode.mode == 0 && constants.stageIndex == 1) {
    if (originalDepth > 0.99) {
      discard;
    }
    outColor = vec4(vec3(originalDepth * 1.45f), 1.0);
  } else if (viewMode.mode == 1 && constants.stageIndex == 1) {
    if (blurValue > 0.99) {
      discard;
    }
    outColor = vec4(vec3(blurValue * 1.45f), 1.0);
  } else if(viewMode.mode == 2 && constants.stageIndex == 1) {
    float volume = float(texture(volumeImage, coord).x);
    if (originalDepth > 0.99f) {
      discard;
    }
    outColor = vec4(vec3(volume * 0.05f), 1.0);
  } else if (viewMode.mode == 3 && constants.stageIndex == 1) {
    ivec2 iCoord = ivec2(coord.x * model.windowSize.x,
                         coord.y * model.windowSize.y);
    float bluredVolume = imageLoad(bluredVolumeImage, iCoord).x;
    if (blurValue > 0.99f) {
      discard;
    }
    outColor = vec4(vec3(min(bluredVolume * 0.5f, 0.5f) + 0.25), 1.0f);
  } else if (viewMode.mode == 4 && constants.stageIndex == 1) {
    //if (originalDepth > 0.999) {
    //  discard;
    //}
    outColor = vec4(getNormal() * 0.5f + vec3(0.5), 1.0);
  } else if (viewMode.mode == 5 &&
             constants.stageIndex == 1) {
    if (blurValue > 0.99) {
      discard;
    }
    outColor = vec4(clamp(blurValue, 0.2, 0.75) * model.color.xyz, 1.0);
  } else if (viewMode.mode == 6) {
    if (constants.stageIndex == 0) {
      outColor = vec4(vec3(texture(cubemap, cubePos).xyz), 1.0);
    } else {
      if (blurValue > 0.99) {
        discard;
      }

      ivec2 iCoord = ivec2(coord.x * model.windowSize.x,
                       coord.y * model.windowSize.y);
      float volumeValue = imageLoad(bluredVolumeImage, iCoord).x; //  * 0.75f + 0.2f

      vec4 viewPos = vec4(getEyePos(coord), 1.0f);
      vec4 worldPos = inverse((mvp.view)) * viewPos;
      vec3 dir = worldPos.xyz - mvp.cameraPos;
      vec3 r = refract(dir, getNormal(), 1.0f/1.33f);
      vec3 background = texture(cubemap, dir + viewMode.refractionStrength * volumeValue * r).xyz;

      vec3 light = calcLight();
      outColor = vec4(light + (1.0f - volumeValue + 0.6f) / 2.0f * background * viewMode.refraction, min(0.95f, imageLoad(bluredVolumeImage, iCoord).x * 0.75f + 0.4f) * viewMode.transparency);
    }
  } else {
    discard;
  }
}