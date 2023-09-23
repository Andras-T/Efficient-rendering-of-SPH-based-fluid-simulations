#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"

class Window {

private:
  GLFWwindow *window;
  GLFWimage icon;
  bool fullScreen = false;

public:
  static double lastTime;
  static bool framebufferResized;

  void init();

  void changeDisplayMode();

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);

  void cleanup();

  GLFWwindow *get_GLFW_Window() { return window; }
};

#endif // WINDOW_H
