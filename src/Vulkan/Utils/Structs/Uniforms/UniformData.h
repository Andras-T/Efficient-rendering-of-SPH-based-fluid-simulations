#pragma once

#include "../InputState.h"
#include "Attributes.h"
#include "BlurSettings.h"
#include "Model.h"
#include "Transformations.h"
#include "ViewMode.h"
#include "imgui.h"

struct UniformData {
  ImGuiIO io;
  Attributes attributes;
  Transformations transformations;
  Model model;
  BlurSettings blurSettings;
  ViewMode viewMode;
  bool wall = false;
  bool resetRequest = false;
};
