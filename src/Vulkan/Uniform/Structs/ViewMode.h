#pragma once

struct ViewMode {
  int mode = 6;
  float shininess = 20.0;
  float ambient = 0.25;
  float lightStrength = 0.15;
  float lightFOV = 60;
  float reflection = 0.1f;
  float maxDepth = 0.67f;
  float transparency = 0.9f;
  float refraction = 0.95f;
  float refractionStrength = 0.1f;
};