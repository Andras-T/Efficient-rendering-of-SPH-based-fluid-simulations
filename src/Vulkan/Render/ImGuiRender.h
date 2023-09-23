#ifndef IMGUIRENDER_H
#define IMGUIRENDER_H

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
  std::vector<UniformData> uniformData;
  std::vector<FluidInstance *> instances;

public:
  void init(Window &window) {
    this->window = &window;
    // instances.resize(100);
  }

  void addInstance(FluidInstance &instance) {
    instances.push_back(&instance);
    uniformData.push_back(UniformData{});
  }

  void removeLastInstance() {
    if (activeInstance == instances.size() - 1 && activeInstance > 0)
      activeInstance--;
    instances.pop_back();
    uniformData.pop_back();
  }

  void draw(VkCommandBuffer &commandBuffer);

  void createMenuBar();

  void createTransformationsMenu(int width, int height);

  void createAppearanceMenu(int width, int height);

  void createPlayButton(int width, int height);

  void createJoyStick(int width, int height, int dockingWindowWidth);

  void menuShortcuts();

  std::vector<UniformData> &getUniformData() { return uniformData; };

  InputState &getInputState() { return inputState; }
};

#endif // IMGUIRENDER_H
