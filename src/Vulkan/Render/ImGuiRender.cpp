#include "ImGuiRender.h"
#include "imgui_internal.h"

ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

ImVec2 operator+=(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

ImVec2 operator*=(const ImVec2 &lhs, const float &rhs) {
  return {lhs.x + rhs, lhs.y + rhs};
}

ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

ImVec2 operator*(const ImVec2 &lhs, const float &rhs) {
  return {lhs.x * rhs, lhs.y * rhs};
}

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

  for (auto &data : uniformData)
    data.io = ImGui::GetIO();

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

  if (!instances.empty()) {

    ImGui::SliderFloat("Particle size", &uniformData[activeInstance].model.size,
                       1.0f, 100.0f);
    ImGui::SliderFloat("Wall particle size",
                       &uniformData[activeInstance].model.wallSize, 1.0f,
                       100.0f);

    ImGui::Checkbox("Wall", &uniformData[activeInstance].wall);
    uniformData[activeInstance].model.wall =
        uniformData[activeInstance].wall ? 1 : 0;

    ImGui::Spacing();

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Color Picker")) {
      ImGui::ColorPicker4("Particle color",
                          (float *)&uniformData[activeInstance].model.color);

      ImGui::Spacing();

      ImGui::ColorPicker4(
          "Wall color", (float *)&uniformData[activeInstance].model.wallColor);
    }
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
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Selection")) {
        for (int i = 0; i < instances.size(); ++i) {
          if (ImGui::MenuItem(
                  ("Select instance (" + std::to_string(i + 1) + ") ").c_str(),
                  i < 9 ? std::to_string(i + 1).c_str() : "")) {
            activeInstance = i;
          }
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
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_1, ImGuiKeyOwner_Any))
    activeInstance = 0;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_2, ImGuiKeyOwner_Any) &&
           instances.size() >= 2)
    activeInstance = 1;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_3, ImGuiKeyOwner_Any) &&
           instances.size() >= 3)
    activeInstance = 2;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_4, ImGuiKeyOwner_Any) &&
           instances.size() >= 4)
    activeInstance = 3;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_5, ImGuiKeyOwner_Any) &&
           instances.size() >= 5)
    activeInstance = 4;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_6, ImGuiKeyOwner_Any) &&
           instances.size() >= 6)
    activeInstance = 5;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_7, ImGuiKeyOwner_Any) &&
           instances.size() >= 7)
    activeInstance = 6;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_8, ImGuiKeyOwner_Any) &&
           instances.size() >= 8)
    activeInstance = 7;
  else if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
           ImGui::IsKeyPressed(ImGuiKey_9, ImGuiKeyOwner_Any) &&
           instances.size() >= 9)
    activeInstance = 8;
  inputState.fixedCamPos = instances[activeInstance]->getCenter();
}

void ImGuiRender::createTransformationsMenu(int width, int height) {
  float posY = ImGui::GetFrameHeight();
  float posX = 0;
  float sizeX = width / 4;
  float sizeY = height - posY - ImGui::GetFrameHeight();

  ImGui::SetNextWindowPos(ImVec2(posX, posY));
  ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
  ImGui::SetNextWindowBgAlpha(0.5f);

  {
    ImGui::Begin("Transformations", nullptr, ImGuiWindowFlags_NoResize);

    ImGui::Checkbox("Free camera", &inputState.freeCam);
    ImGui::SliderFloat("Speed", &inputState.cameraSpeed, 0.01f, 10.0f);
    if (!instances.empty()) {
      ImGui::SliderFloat("Scale",
                         &uniformData[activeInstance].transformations.s, 0.001f,
                         5.0f);
      ImGui::SliderFloat("Scale X",
                         &uniformData[activeInstance].transformations.scale.x,
                         0.001f, 5.0f);
      ImGui::SliderFloat("Scale Y",
                         &uniformData[activeInstance].transformations.scale.y,
                         0.001f, 5.0f);
      ImGui::SliderFloat("Scale Z",
                         &uniformData[activeInstance].transformations.scale.z,
                         0.001f, 5.0f);
      ImGui::SliderFloat(
          "Rotate", &uniformData[activeInstance].transformations.rotations.z,
          0.0f, 720.0f);
      ImGui::SliderFloat3(
          "Translate",
          (float *)&uniformData[activeInstance].transformations.translate,
          -10.0f, 10.0f);

      ImGui::Spacing();

      ImGui::DragFloat("Smoothing length",
                       &uniformData[activeInstance].attributes.smootingLength,
                       0.001f, 0.0f, 0.25f, "%.3f", 0);
      ImGui::DragFloat("particle mass",
                       &uniformData[activeInstance].attributes.mass, 0.1f, 0.0f,
                       500.0f, "%.3f", 0);
      if (uniformData[activeInstance].attributes.mass == 0)
        uniformData[activeInstance].attributes.mass = 0.1f;
      ImGui::DragFloat("Equation of state constant",
                       &uniformData[activeInstance].attributes.stateConstant,
                       0.5f, 0.0f, 5000.0f, "%.1f", 0);
      ImGui::SliderFloat("gravity",
                         &uniformData[activeInstance].attributes.gravity,
                         -500.0f, 1000.0f);
      ImGui::DragFloat("damping",
                       &uniformData[activeInstance].attributes.damping, 0.5f,
                       0.0f, 100.0f, "%.1f", 0);

      ImGui::Spacing();
      ImGui::Spacing();
      static std::string buttonName;
      buttonName =
          uniformData[activeInstance].attributes.stop == 0 ? "Stop" : "Resume";
      if (ImGui::Button(buttonName.c_str(), ImVec2(width / 4, 40))) {
        if (!firstTime) {
          uniformData[activeInstance].attributes.stop =
              (uniformData[activeInstance].attributes.stop + 1) % 2;
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
        uniformData[activeInstance].transformations = t;
        InputState i;
        inputState = i;
        if (!firstTime && uniformData[activeInstance].attributes.stop == 0)
          a.stop = 0;
        uniformData[activeInstance].attributes = a;
        uniformData[activeInstance].transformations.translate = glm::vec3(0.0);
      }

      ImGui::Spacing();

      if (ImGui::Button("Reset  transformations", ImVec2(width / 4, 40))) {
        Transformations t;
        uniformData[activeInstance].transformations = t;
        InputState i;
        inputState = i;
        uniformData[activeInstance].transformations.translate = glm::vec3(0.0);
      }

      ImGui::Spacing();

      if (ImGui::Button("Reset  attributes", ImVec2(width / 4, 40))) {
        Attributes a;
        if (!firstTime && uniformData[activeInstance].attributes.stop == 0)
          a.stop = 0;
        uniformData[activeInstance].attributes = a;
      }
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
      for (auto &data : uniformData)
        data.attributes.stop = 0;
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
    uniformData[activeInstance].transformations.rotations.x += -movement.y / 20;
    uniformData[activeInstance].transformations.rotations.y += -movement.x / 20;
  }
}