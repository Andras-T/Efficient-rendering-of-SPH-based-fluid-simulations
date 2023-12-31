#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "ImGuiRender.h"
#include "Vulkan/Utils/Structs/ImVecUtils.h"
#include "imgui_internal.h"

void ImGuiRender::draw(VkCommandBuffer& commandBuffer) {
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
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void ImGuiRender::createAppearanceMenu(int width, int height) {
	float posY = ImGui::GetFrameHeight();
	float posX = 4 * width / 5;
	float sizeX = width / 5;
	float sizeY = height - posY - ImGui::GetFrameHeight();

	ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
	ImGui::SetNextWindowPos(ImVec2(posX, posY));
	ImGui::SetNextWindowBgAlpha(0.5f);

	ImGui::Begin("Appearance", nullptr, ImGuiWindowFlags_NoResize);

	ImGui::SliderFloat("Font size", &ImGui::GetIO().FontGlobalScale, 0.5f, 2.0f);

	ImGui::Spacing();

	ImGui::Checkbox("Wall", &uniformData.wall);
	uniformData.model.wall = uniformData.wall ? 1 : 0;

	ImGui::Spacing();

	ImGui::DragFloat("Sphere size", &uniformData.model.particleRadius, 0.005f, 0.01f,
		0.1f, "%.3f", 0);

	ImGui::Spacing();

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Color Picker")) {
		ImGui::Spacing();
		ImGui::ColorPicker3("Background Color", (float*)backgroundColor.float32);
		ImGui::Spacing();
		ImGui::ColorPicker4("Fluid color", (float*)&(uniformData.model.color));
	}

	ImGui::Spacing();

	ImGui::Text("Lightning");

	ImGui::Spacing();

	ImGui::DragFloat("Light FoV", &uniformData.viewMode.lightFOV, 0.5f, 5.0f,
		90.0f, "%.1f", 0);

	ImGui::DragFloat("Shininess", &uniformData.viewMode.shininess, 0.1f, 0.001f,
		500.0f, "%.1f", 0);

	ImGui::DragFloat("Ambient light", &uniformData.viewMode.ambient, 0.05f,
		0.001f, 1.0f, "%.2f", 0);

	ImGui::DragFloat("Light strength", &uniformData.viewMode.lightStrength, 0.05f,
		0.001f, 5.0f, "%.2f", 0);

	ImGui::DragFloat("Reflection", &uniformData.viewMode.reflection, 0.1f, 0.0f,
		1.0f, "%.2f", 0);
	ImGui::DragFloat("Max Depth", &uniformData.viewMode.maxDepth, 0.01f, 0.0f,
		1.0f, "%.2f", 0);

	ImGui::DragFloat("Transparency", &uniformData.viewMode.transparency, 0.05f, 0.0f,
		4.0f, "%.2f", 0);

	ImGui::DragFloat("Refraction", &uniformData.viewMode.refraction, 0.05f, 0.0f,
		4.0f, "%.2f", 0);

	ImGui::DragFloat("Refraction Strength", &uniformData.viewMode.refractionStrength, 0.05f, 0.0f,
		1.0f, "%.2f", 0);

	ImGui::Spacing();

	// View Modes
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
		const auto child_size = ImVec2(355, 160);
		ImGui::BeginChild("ChildDrawList", child_size, true,
			ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			ImGui::TextUnformatted("View modes");
			ImGui::EndMenuBar();
		}
		const auto draw_list_size = ImVec2(355, 160);

		constexpr int itemSize = 7;
		const char* items[itemSize] = { "Depth view", "Blured depth view", "volume view", "Blured volume view",
																	 "Normal vector view", "Colored view",
																	 "Lights & Background" };

		if (ImGui::BeginListBox("Modes", draw_list_size)) {
			if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
				if (uniformData.viewMode.mode < itemSize - 1) {
					++uniformData.viewMode.mode;
				}
			}
			if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
				if (uniformData.viewMode.mode > 0) {
					--uniformData.viewMode.mode;
				}
			}

			for (int n = 0; n < itemSize; ++n) {
				const bool is_selected = (uniformData.viewMode.mode == n);
				if (ImGui::Selectable(items[n], is_selected)) {
					uniformData.viewMode.mode = n;
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::End();
}

void ImGuiRender::createMenuBar() {
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
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

			outfile << Utils::time().str() << "," << std::to_string(ImGui::GetIO().Framerate) << "\n";

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
}

void ImGuiRender::createTransformationsMenu(int width, int height) {
	float posY = ImGui::GetFrameHeight();
	float posX = 0;
	float sizeX = width / 4;
	float sizeY = height - posY - ImGui::GetFrameHeight();

	ImGui::SetNextWindowPos(ImVec2(posX, posY));
	ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
	ImGui::SetNextWindowBgAlpha(0.5f);

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize);

	ImGui::Checkbox("Free camera", &inputState.freeCam);
	ImGui::Spacing();
	ImGui::SliderFloat("Speed", &inputState.cameraSpeed, 0.1f, 10.0f);
	ImGui::DragFloat("Far plane", &inputState.far, 0.01f, 0.1f, 10.0f, "%.3f", 0);
	ImGui::DragFloat("Near plane", &inputState.near, 0.001f, 0.001f, 2.0f, "%.4f", 0);
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Model Settings: ");
		ImGui::Spacing();

		ImGui::SliderFloat("Scale", &uniformData.transformations.s, 0.001f, 5.0f);

		ImGui::SliderFloat("Scale X", &uniformData.transformations.scale.x, 0.001f,
			1.0f);
		ImGui::SliderFloat("Scale Y", &uniformData.transformations.scale.y, 0.001f,
			1.0f);
		ImGui::SliderFloat("Scale Z", &uniformData.transformations.scale.z, 0.001f,
			1.0f);
		ImGui::SliderFloat("Rotate X", &uniformData.transformations.rotations.x,
			-360.0f, 720.0f);
		ImGui::SliderFloat("Rotate Y", &uniformData.transformations.rotations.y,
			-360.0f, 720.0f);
		ImGui::SliderFloat("Rotate Z", &uniformData.transformations.rotations.z,
			-360.0f, 720.0f);
		ImGui::SliderFloat3("Translate",
			(float*)&uniformData.transformations.translate,
			-0.5f, 0.5f);
		ImGui::DragFloat3("Light Position", (float*)&uniformData.transformations.lightPosition,
			0.1f, -2.0, 2.0, "%.2f", 0);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Particle Settings: ");
		ImGui::Spacing();

		ImGui::DragFloat("Smoothing length", &uniformData.attributes.smootingLength,
			0.001f, 0.001f, 0.5f, "%.3f", 0);
		ImGui::DragFloat("Particle mass", &uniformData.attributes.mass, 0.01f, 0.0f,
			1000.0f, "%.2f", 0);
		ImGui::DragFloat("Equation of state constant",
			&uniformData.attributes.stateConstant, 0.001f, 0.01f,
			5000.0f, "%.3f", 0);
		ImGui::DragFloat("Gravity", &uniformData.attributes.gravity, 0.5f, -500.0f,
			2500.0f, "%.2f", 0);
		ImGui::DragFloat("Damping Acceleration",
			&uniformData.attributes.dampingAcceleration, 0.0001f, 0.0f,
			300.0f, "%.4f", 0);
		ImGui::DragFloat("Damping Velocity",
			&uniformData.attributes.dampingVelocity, 0.0001f, 0.0f,
			300.0f, "%.4f", 0);
		ImGui::DragFloat("Polytropic index",
			&uniformData.attributes.polytropicIndex, 0.01f, 0.001f,
			10.0f, "%.3f", 0);
		ImGui::DragFloat("Time step", &uniformData.attributes.timeStep, 0.001f,
			0.001f, 5.0f, "%.3f", 0);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Blur Settings: ");
		ImGui::Spacing();

		ImGui::DragFloat("Kernel size",
			&uniformData.blurSettings.kernelSize, 1.0f, 1.0f, 50.0f,
			"%.0f", 0);
		ImGui::DragFloat("Sigma",
			&uniformData.blurSettings.sigma, 0.1f, 0.1f, 60.0f,
			"%.1f", 0);
		ImGui::DragFloat("Blur direction X", &uniformData.blurSettings.blurDir.x,
			1.0f, 1.0f, 3.0f, "%.1f", 0);
		ImGui::DragFloat("Blur direction Y", &uniformData.blurSettings.blurDir.y,
			1.0f, 1.0f, 3.0f, "%.1f", 0);
		ImGui::Spacing();
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

		if (ImGui::Button("Reset attributes & transformations",
			ImVec2(width / 4, 40))) {
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

		if (ImGui::Button("Reset transformations", ImVec2(width / 4, 40))) {
			Transformations t;
			uniformData.transformations = t;
			InputState i;
			inputState = i;
			uniformData.transformations.translate = glm::vec3(0.0);
		}

		ImGui::Spacing();

		if (ImGui::Button("Reset attributes", ImVec2(width / 4, 40))) {
			Attributes a;
			if (!firstTime && uniformData.attributes.stop == 0)
				a.stop = 0;
			uniformData.attributes = a;
		}
		ImGui::Spacing();

		if (ImGui::Button("Reset position", ImVec2(width / 4, 40))) {
			uniformData.resetRequest = true;
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
		ImDrawList* drawList = ImGui::GetWindowDrawList();
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

	ImDrawList* drawList = ImGui::GetWindowDrawList();
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