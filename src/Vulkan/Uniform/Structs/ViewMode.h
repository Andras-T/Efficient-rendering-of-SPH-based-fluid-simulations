#pragma once

struct ViewMode {
  int mode = 5;
  int viewOrWorldSpace = 1;
  float shininess = 20.0;
  float ambient = 0.40;
  float lightStrength = 0.4;
  float lightFOV = 25;
  float pad1;
  float pad2;
};