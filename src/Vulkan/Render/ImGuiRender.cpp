#include "ImGuiRender.h"
#include "imgui_internal.h"
#include "../Utils/Structs/ImVecUtils.h"

void ImGuiRender::draw(VkCommandBuffer &commandBuffer) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  int width, height;
  glfwGetWindowSize(window->get_GLFW_Window(), &width, &height);

  createMenuBar();

  if (showTransformationsMenu)
    createTransformationsMenu(width, height);

  createPlayButton(width, height);

  if (showAppearanceMenu)
    createAppearanceMenu(width, height);

  uniformData.io = ImGui::GetIO();

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void ImGuiRender::createAppearanceMenu(int width, int height) {
  float posY = ImGui::GetFrameHeight();
  float posX = 3 * width / 4;
  float sizeX = width / 4;
  float sizeY = height - posY - ImGui::GetFrameHeight();

  ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
  ImGui::SetNextWindowPos(ImVec2(posX, posY));
  ImGui::SetNextWindowBgAlpha(0.5f);

  ImGui::Begin("Appearance", nullptr, ImGuiWindowFlags_NoResize);

  ImGui::SliderFloat("Font size", &ImGui::GetIO().FontGlobalScale, 0.5f, 2.0f);

  ImGui::SliderFloat3("Font size", &ImGui::GetIO().FontGlobalScale, 0.5f, 2.0f);

  ImGui::SliderFloat3("Background Color", (float *)backgroundColor.float32,
                      0.0f, 1.0f);

  ImGui::Checkbox("Wall", &uniformData.wall);
  uniformData.model.wall = uniformData.wall ? 1 : 0;

  ImGui::Spacing();

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::CollapsingHeader("Color Picker")) {
    ImGui::ColorPicker3("Particle color", (float *)&uniformData.model.color);

    ImGui::Spacing();

    ImGui::ColorPicker3("Wall color", (float *)&uniformData.model.wallColor);
  }
  ImGui::End();
}

void ImGuiRender::createMenuBar() {
  ImGuiViewportP *viewport = (ImGuiViewportP *)(void *)ImGui::GetMainViewport();
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_MenuBar;
  float height = ImGui::GetFrameHeight();

  if (ImGui::BeginViewportSideBar("Top bar", viewport, ImGuiDir_Up, height,
                                  window_flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Menu")) {
        if (ImGui::MenuItem("Change display mode", "Alt+Enter"))
          window->changeDisplayMode();
        if (ImGui::MenuItem("Exit", "Alt+Q")) {
          glfwSetWindowShouldClose(window->get_GLFW_Window(), true);
        }
        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Show/remove transformation menu", "Alt+1")) {
          showTransformationsMenu = !showTransformationsMenu;
        }
        if (ImGui::MenuItem("Show/remove appearance menu", "Alt+2")) {
          showAppearanceMenu = !showAppearanceMenu;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
    ImGui::End();
  }

  if (ImGui::BeginViewportSideBar("Bottom bar", viewport, ImGuiDir_Down, height,
                                  window_flags)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::Text("Status: ");
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0 / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
      ImGui::EndMenuBar();
    }
    ImGui::End();
  }

  menuShortcuts();
}

void ImGuiRender::menuShortcuts() {
  if (ImGui::GetIO().KeyMods == ImGuiModFlags_Alt &&
      ImGui::IsKeyPressed(ImGuiKey_1, ImGuiKeyOwner_Any))
    showTransformationsMenu = !showTransformationsMenu;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Alt &&
           ImGui::IsKeyPressed(ImGuiKey_2, ImGuiKeyOwner_Any))
    showAppearanceMenu = !showAppearanceMenu;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Alt &&
           ImGui::IsKeyPressed(ImGuiKey_Q, ImGuiKeyOwner_Any))
    glfwSetWindowShouldClose(window->get_GLFW_Window(), GLFW_TRUE);
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Alt &&
           ImGui::IsKeyPressed(ImGuiKey_Enter, ImGuiKeyOwner_Any))
    window->changeDisplayMode();
  inputState.fixedCamPos = instance->getCenter();
}

void ImGuiRender::createTransformationsMenu(int width, int height) {
  float posY = ImGui::GetFrameHeight();
  float posX = 0;
  float sizeX = width / 4;
  float sizeY = height - posY - ImGui::GetFrameHeight();

  ImGui::SetNextWindowPos(ImVec2(posX, posY));
  ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
  ImGui::SetNextWindowBgAlpha(0.5f);

  ImGui::Begin("Transformations", nullptr, ImGuiWindowFlags_NoResize);

  ImGui::Checkbox("Free camera", &inputState.freeCam);
  ImGui::SliderFloat("Speed", &inputState.cameraSpeed, 0.01f, 10.0f);
  {
    ImGui::SliderFloat("Scale", &uniformData.transformations.s, 0.001f, 5.0f);
    ImGui::SliderFloat("Scale X", &uniformData.transformations.scale.x, 0.001f,
                       5.0f);
    ImGui::SliderFloat("Scale Y", &uniformData.transformations.scale.y, 0.001f,
                       5.0f);
    ImGui::SliderFloat("Scale Z", &uniformData.transformations.scale.z, 0.001f,
                       5.0f);
    ImGui::SliderFloat("Rotate", &uniformData.transformations.rotations.z, 0.0f,
                       720.0f);
    ImGui::SliderFloat3("Translate",
                        (float *)&uniformData.transformations.translate, -10.0f,
                        10.0f);

    ImGui::Spacing();

    ImGui::DragFloat("Smoothing length", &uniformData.attributes.smootingLength,
                     0.001f, 0.0f, 0.25f, "%.3f", 0);
    ImGui::DragFloat("particle mass", &uniformData.attributes.mass, 0.1f, 0.0f,
                     500.0f, "%.3f", 0);
    if (uniformData.attributes.mass == 0)
      uniformData.attributes.mass = 0.1f;
    ImGui::DragFloat("Equation of state constant",
                     &uniformData.attributes.stateConstant, 0.5f, 0.0f, 5000.0f,
                     "%.1f", 0);
    ImGui::SliderFloat("gravity", &uniformData.attributes.gravity, -500.0f,
                       1000.0f);
    ImGui::DragFloat("damping", &uniformData.attributes.damping, 0.5f, 0.0f,
                     100.0f, "%.1f", 0);

    ImGui::Spacing();
    ImGui::Spacing();
    static std::string buttonName;
    buttonName = uniformData.attributes.stop == 0 ? "Stop" : "Resume";
    if (ImGui::Button(buttonName.c_str(), ImVec2(width / 4, 40))) {
      if (!firstTime) {
        uniformData.attributes.stop = (uniformData.attributes.stop + 1) % 2;
        if (buttonName == "Stop")
          buttonName = "Resume";
        else
          buttonName = "Stop";
      }
    }

    ImGui::Spacing();

    if (ImGui::Button("Reset", ImVec2(width / 4, 40))) {
      Attributes a;
      Transformations t;
      uniformData.transformations = t;
      InputState i;
      inputState = i;
      if (!firstTime && uniformData.attributes.stop == 0)
        a.stop = 0;
      uniformData.attributes = a;
      uniformData.transformations.translate = glm::vec3(0.0);
    }

    ImGui::Spacing();

    if (ImGui::Button("Reset  transformations", ImVec2(width / 4, 40))) {
      Transformations t;
      uniformData.transformations = t;
      InputState i;
      inputState = i;
      uniformData.transformations.translate = glm::vec3(0.0);
    }

    ImGui::Spacing();

    if (ImGui::Button("Reset  attributes", ImVec2(width / 4, 40))) {
      Attributes a;
      if (!firstTime && uniformData.attributes.stop == 0)
        a.stop = 0;
      uniformData.attributes = a;
    }
    ImGui::Spacing();

    createJoyStick(width, height,
                   width / 4 + ImGui::GetStyle().WindowPadding.x);

    ImGui::End();
  }
}

void ImGuiRender::createPlayButton(int width, int height) {
  if (firstTime) {
    ImGui::Begin("Invisible Window", nullptr,
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize);
    ImVec2 size(width / 8, height / 6);
    ImGui::SetWindowPos((ImGui::GetIO().DisplaySize) * 0.5f - size * 0.5);
    ImGui::SetWindowSize(size * 1.1);

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    if (ImGui::Button("Play", size)) {
      uniformData.attributes.stop = 0;
      firstTime = false;
    }

    ImVec2 buttonPos = ImGui::GetItemRectMin();
    ImVec2 buttonSize = ImGui::GetItemRectSize();

    // Calculate triangle points
    ImVec2 p1(buttonPos.x + buttonSize.x * 0.3f,
              buttonPos.y + buttonSize.y * 0.2f);
    ImVec2 p2(buttonPos.x + buttonSize.x * 0.8f,
              buttonPos.y + buttonSize.y * 0.5f);
    ImVec2 p3(buttonPos.x + buttonSize.x * 0.3f,
              buttonPos.y + buttonSize.y * 0.8f);

    ImU32 triangleColor = ImGui::GetColorU32(ImGuiCol_Text);

    // Draw the triangle
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddTriangleFilled(p1, p2, p3, triangleColor);
    ImGui::End();
  }
}

void ImGuiRender::createJoyStick(int width, int height,
                                 int dockingWindowWidth) {
  float radius = 100;
  ImVec2 center = ImVec2(dockingWindowWidth / 2, height - radius - 50);
  ImVec2 joystickPos = center;
  ImVec2 movement;

  bool joystickActive = false;

  if (ImGui::IsWindowFocused() &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImVec2 delta = mousePos - center;

    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
    if (dist < radius) {
      joystickActive = true;
      if (dist < radius / 2)
        joystickPos = mousePos;
      else {
        joystickPos = center + (delta * (1 / dist)) * (radius / 2);
      }
      movement = joystickPos - center;
    }
  }

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  drawList->AddCircle(center, radius, ImColor(255, 255, 255, 128), 32);
  drawList->AddCircleFilled(joystickPos, radius * 0.5f,
                            ImColor(255, 255, 255, 192), 32);

  if (joystickActive) {
    float dist = sqrtf(movement.x * movement.x + movement.y * movement.y);
    movement = movement * (1 / dist);
    uniformData.transformations.rotations.x += -movement.y / 10;
    uniformData.transformations.rotations.y += -movement.x / 10;
  }
}