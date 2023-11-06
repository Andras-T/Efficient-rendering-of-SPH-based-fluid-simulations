#pragma once

struct ViewMode {
  int mode = 4;
  float shininess = 20.0;
  float ambient = 0.60;
  float lightStrength = 0.25;
  float lightFOV = 10;
  float reflection = 0.10f;
  float maxDepth = 0.97f;
  float pad2;
};