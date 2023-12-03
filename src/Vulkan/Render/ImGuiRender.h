#pragma once

#include "Vulkan/FluidInstance.h"
#include "Vulkan/Uniform/Structs/UniformData.h"
#include "Vulkan/Utils/Utils.h"
#include "Vulkan/Window/Window.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <string>
#include <fstream>

class ImGuiRender {

private:
	bool firstTime = true;
	bool showTransformationsMenu = true;
	bool showAppearanceMenu = true;
	int activeInstance = 0;

	std::ofstream outfile;

	Window* window;
	InputState inputState;
	UniformData& uniformData;
	FluidInstance* instance;

public:
	VkClearColorValue backgroundColor = { 0.25f, 0.25f, 0.25f, 1.0f };

	ImGuiRender(UniformData& uniformData) : uniformData(uniformData) {}

	void init(Window& window, FluidInstance* instance) {
		this->window = &window;
		this->instance = instance;
		outfile.open("fps_log.txt", std::ios_base::app); // append instead of overwrite
	}

	~ImGuiRender() { outfile.close(); }

	void draw(VkCommandBuffer& commandBuffer);

	void createMenuBar();

	void createTransformationsMenu(int width, int height);

	void createAppearanceMenu(int width, int height);

	void createPlayButton(int width, int height);

	void createJoyStick(int width, int height, int dockingWindowWidth);

	void menuShortcuts();

	InputState& getInputState() { return inputState; }
};
