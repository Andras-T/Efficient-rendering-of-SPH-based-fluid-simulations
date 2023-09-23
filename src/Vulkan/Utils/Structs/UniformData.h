#ifndef UNIFORMDATA_H
#define UNIFORMDATA_H

#include "Attributes.h"
#include "InputState.h"
#include "Model.h"
#include "Transformations.h"
#include "imgui.h"

struct UniformData {
  ImGuiIO io;
  Attributes attributes;
  Transformations transformations;
  Model model;
  bool wall = false;
};

#endif // UNIFORMDATA_H
