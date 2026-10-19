#ifndef SPH_SCREEN_GLSL
#define SPH_SCREEN_GLSL

const float kPi = 3.14159265358979323846;

vec3 ViewDir(vec2 uv, mat4 projection) {
  const vec2 ndc = uv * 2.0 - 1.0;
  return vec3(ndc.x / projection[0][0], ndc.y / projection[1][1], -1.0);
}

vec3 ViewPosition(vec2 uv, float eyeDepth, mat4 projection) {
  return ViewDir(uv, projection) * eyeDepth;
}

vec2 EquirectUv(vec3 dir) {
  const vec3 d = normalize(dir);
  return vec2(atan(d.z, d.x) / (2.0 * kPi) + 0.5, acos(clamp(d.y, -1.0, 1.0)) / kPi);
}

vec3 SampleEquirect(sampler2D hdri, vec3 dir) { return texture(hdri, EquirectUv(dir)).rgb; }

vec3 AcesTonemap(vec3 color) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 Heatmap(float t) {
  const float x = clamp(t, 0.0, 1.0);
  return vec3(clamp(1.5 * x - 0.1, 0.0, 1.0), clamp(1.5 * x, 0.0, 1.0) * (1.0 - 0.4 * x),
              clamp(1.0 - 1.6 * x + 0.2, 0.0, 1.0));
}

#endif
