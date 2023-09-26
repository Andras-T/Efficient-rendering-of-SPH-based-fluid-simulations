#pragma once

#include "../FluidInstance.h"
#include "../Utils/Structs/UniformData.h"
#include "../Utils/Utils.h"
#include "../Window/Window.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <string>

class ImGuiRender {

private:
  bool firstTime = true;
  bool showTransformationsMenu = true;
  bool showAppearanceMenu = true;
  int activeInstance = 0;

  Window *window;
  InputState inputState;
  UniformData uniformData;
  FluidInstance *instance;

public:
  VkClearColorValue backgroundColor = {0.25f, 0.25f, 0.25f, 1.0f};

  void init(Window &window, FluidInstance *instance) {
    this->window = &window;
    this->instance = instance;
  }

  void draw(VkCommandBuffer &commandBuffer);

  void createMenuBar();

  void createTransformationsMenu(int width, int height);

  void createAppearanceMenu(int width, int height);

  void createPlayButton(int width, int height);

  void createJoyStick(int width, int height, int dockingWindowWidth);

  void menuShortcuts();

  UniformData &getUniformData() { return uniformData; };

  InputState &getInputState() { return inputState; }
};
